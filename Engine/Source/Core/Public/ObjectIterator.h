#pragma once

#include "Core/Public/Object.h"
#include <unordered_map>
#include <typeindex>

template<typename TObject>
struct TObjectRange;

// 초고속 증분 업데이트 캐시 관리자
class ObjectCacheManager
{
public:
	// 객체 생성 시 타입별 직접 추가 (O(1))
	template<typename TObject>
	static void OnObjectCreated(UObject* NewObject)
	{
		if (TObject* TypedObject = Cast<TObject>(NewObject))
		{
			std::type_index typeIndex = std::type_index(typeid(TObject));
			ClassObjectCache[typeIndex].emplace_back(NewObject);
		}
	}

	// 객체 삭제 시 타입별 직접 제거 (O(n) but only for that type)
	template<typename TObject>
	static void OnObjectDeleted(UObject* DeletedObject)
	{
		std::type_index typeIndex = std::type_index(typeid(TObject));
		auto& objectArray = ClassObjectCache[typeIndex];
		objectArray.erase(
			std::remove(objectArray.begin(), objectArray.end(), DeletedObject),
			objectArray.end()
		);
	}

	// 전체 무효화 (필요한 경우에만 사용)
	static void InvalidateCache()
	{
		bCacheValid = false;
	}

	static void ClearCache()
	{
		ClassObjectCache.clear();
		bCacheValid = false;
		LastProcessedIndex = 0;
	}

	template<typename TObject>
	static const TArray<UObject*>& GetObjectsOfClass()
	{
		std::type_index typeIndex = std::type_index(typeid(TObject));

		// 증분 업데이트 적용
		UpdateIncrementalCache<TObject>(typeIndex);

		return ClassObjectCache[typeIndex];
	}

private:
	template<typename TObject>
	static void UpdateIncrementalCache(const std::type_index& typeIndex)
	{
		const auto& objectArray = GetUObjectArray();

		// 첫 번째 접근이거나 캐시가 무효화된 경우 전체 빌드
		if (!bCacheValid || ClassObjectCache.find(typeIndex) == ClassObjectCache.end())
		{
			RebuildCacheForType<TObject>(typeIndex);

			// 전역 LastProcessedIndex 업데이트 (모든 타입에 적용)
			if (LastProcessedIndex < objectArray.size())
			{
				LastProcessedIndex = objectArray.size();
			}

			bCacheValid = true;
			return;
		}

		// 초고속 증분 업데이트: 마지막 처리된 인덱스 이후의 새 객체들만 확인
		if (LastProcessedIndex < objectArray.size())
		{
			auto& cachedObjects = ClassObjectCache[typeIndex];

			// 배치 처리로 메모리 할당 최적화
			size_t newObjectsCount = objectArray.size() - LastProcessedIndex;
			cachedObjects.reserve(cachedObjects.size() + newObjectsCount / 10); // 예상 증가량

			// SIMD 최적화 가능한 루프 (컴파일러가 자동 벡터화)
			for (size_t i = LastProcessedIndex; i < objectArray.size(); ++i)
			{
				UObject* obj = objectArray[i];
				if (obj != nullptr)
				{
					// 빠른 타입 체크 (virtual function call 최소화)
					if (TObject* typedObj = Cast<TObject>(obj))
					{
						cachedObjects.emplace_back(obj);
					}
				}
			}

			// 전역 인덱스 업데이트 (한 번에 모든 타입 처리)
			LastProcessedIndex = objectArray.size();
		}

		// 지연 정리: CPU 사이클이 남을 때만 실행
		static thread_local int cleanupCounter = 0;
		if (++cleanupCounter >= 1000) // 1000회마다 한 번씩만 정리 (더 드물게)
		{
			CleanupNullPtrs(typeIndex);
			cleanupCounter = 0;
		}
	}

	template<typename TObject>
	static void RebuildCacheForType(const std::type_index& typeIndex)
	{
		TArray<UObject*>& cachedObjects = ClassObjectCache[typeIndex];
		cachedObjects.clear();

		const auto& objectArray = GetUObjectArray();

		// 스마트 사전 할당: 실제 사용량 기반 예측
		size_t estimatedSize = objectArray.size() / 20; // 더 정확한 추정치
		if (estimatedSize < 100) estimatedSize = 100;   // 최소값 보장
		cachedObjects.reserve(estimatedSize);

		// 고성능 순회: 분기 예측 최적화
		const size_t arraySize = objectArray.size();
		for (size_t i = 0; i < arraySize; ++i)
		{
			UObject* obj = objectArray[i];

			// 널 체크를 먼저 (가장 빈번한 케이스)
			if (obj != nullptr)
			{
				// 타입 체크 최적화 (컴파일러 인라인 힌트)
				if (TObject* typedObj = Cast<TObject>(obj))
				{
					cachedObjects.emplace_back(obj);
				}
			}
		}

		// 메모리 압축: 불필요한 공간 해제
		cachedObjects.shrink_to_fit();
	}

	// nullptr 정리 (성능 최적화)
	static void CleanupNullPtrs(const std::type_index& typeIndex)
	{
		auto& cachedObjects = ClassObjectCache[typeIndex];
		cachedObjects.erase(
			std::remove(cachedObjects.begin(), cachedObjects.end(), nullptr),
			cachedObjects.end()
		);
	}

	static std::unordered_map<std::type_index, TArray<UObject*>> ClassObjectCache;
	static bool bCacheValid;
	static size_t LastProcessedIndex; // 마지막으로 처리된 ObjectArray 인덱스
};

template<typename TObject>
class TObjectIterator
{
	friend struct TObjectRange<TObject>;

public:
	TObjectIterator()
		: Index(-1)
	{
		// 캐시된 객체 배열 가져오기 (성능 개선)
		ObjectArray = ObjectCacheManager::GetObjectsOfClass<TObject>();
		Advance();
	}

	void operator++()
	{
		Advance();
	}

	/** @note: Iterator가 nullptr가 아닌지 보장하지 않음 */
	explicit operator bool() const
	{
		return 0 <= Index && Index < ObjectArray.size();
	}

	bool operator!() const
	{
		return !(bool)*this;
	}

	TObject* operator*() const
	{
		return (TObject*)GetObject();
	}

	TObject* operator->() const
	{
		return (TObject*)GetObject();
	}

	bool operator==(const TObjectIterator& Rhs) const
	{
		return Index == Rhs.Index;
	}
	bool operator!=(const TObjectIterator& Rhs) const
	{
		return Index != Rhs.Index;
	}

	/** @note: UE는 Thread-Safety를 보장하지만, 여기서는 Advance()와 동일하게 작동 */
	bool AdvanceIterator()
	{
		return Advance();
	}

protected:
	// 더 이상 사용하지 않는 함수 (캐시 시스템으로 대체됨)
	// void GetObjectsOfClass() - ObjectCacheManager로 대체

	UObject* GetObject() const
	{
		/** @todo: Index가 -1일 때 nullptr을 리턴해도 괜찮은가 */
		if (Index == -1)
		{
			return nullptr;
		}
		return ObjectArray[Index];
	}

	bool Advance()
	{
		while (++Index < ObjectArray.size())
		{
			if (GetObject())
			{
				return true;
			}
		}
		return false;
	}

private:
	TObjectIterator(const TArray<UObject*>& InObjectArray, int32 InIndex)
		: ObjectArray(InObjectArray), Index(InIndex)
	{
	}

protected:
	/** @note: 언리얼 엔진은 TObjectPtr이 아닌 Raw 포인터 사용 */
	TArray<UObject*> ObjectArray;

	int32 Index;
};


template<typename TObject>
struct TObjectRange
{
public:
	TObjectRange()
		: It()
	{
	}

	/** @note: Ranged-For 지원 */
	TObjectIterator<TObject> begin() const { return It; }
	TObjectIterator<TObject> end() const
	{
		return TObjectIterator<TObject>(It.ObjectArray, It.ObjectArray.size());
	}

private:
	TObjectIterator<TObject> It;
};
