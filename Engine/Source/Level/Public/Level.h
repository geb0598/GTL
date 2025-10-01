#pragma once
#include "Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Factory/Public/NewObject.h"

#include "Editor/Public/Camera.h"

namespace json { class JSON; }
using JSON = json::JSON;

class AAxis;
class AGizmo;
class AGrid;
class AActor;
class UPrimitiveComponent;
class FFrustumCull;

/**
 * @brief Level Show Flag Enum
 */
enum class EEngineShowFlags : uint64
{
	SF_Primitives = 0x01,
	SF_BillboardText = 0x10,
	SF_Bounds = 0x20,
};

inline uint64 operator|(EEngineShowFlags lhs, EEngineShowFlags rhs)
{
	return static_cast<uint64>(lhs) | static_cast<uint64>(rhs);
}

inline uint64 operator&(uint64 lhs, EEngineShowFlags rhs)
{
	return lhs & static_cast<uint64>(rhs);
}

class ULevel :
	public UObject
{
public:
	ULevel();
	ULevel(const FName& InName);
	~ULevel() override;

	virtual void Init();
	virtual void Update();
	virtual void Render();
	virtual void Cleanup();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

	const TArray<TObjectPtr<AActor>>& GetLevelActors() const { return LevelActors; }

	const TArray<TObjectPtr<UPrimitiveComponent>>& GetLevelPrimitiveComponents() const
	{
		return LevelPrimitiveComponents;
	}

	TArray<TObjectPtr<UPrimitiveComponent>> GetVisiblePrimitiveComponents(UCamera* InCamera);

	void AddLevelPrimitiveComponent(AActor* Actor);
	void InitializeActorsInLevel();

	AActor* SpawnActorToLevel(UClass* InActorClass, const FName& InName = FName::GetNone());
	void RegisterDuplicatedActor(AActor* NewActor);

	bool DestroyActor(AActor* InActor);
	void MarkActorForDeletion(AActor* InActor);

	void SetSelectedActor(AActor* InActor);
	TObjectPtr<AActor> GetSelectedActor() const { return SelectedActor; }

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	// LOD Update System
	void UpdateLODForAllMeshes();
	void TickLODUpdate();

	// Graphics Quality Control
	void SetGraphicsQuality(int32 QualityLevel);

	// LOD Control Functions
	void SetGlobalLODEnabled(bool bEnabled);
	void SetMinLODLevel(int32 MinLevel);
	void SetLODDistance1(float Distance);
	void SetLODDistance2(float Distance);

private:
	TArray<TObjectPtr<AActor>> LevelActors;
	TArray<TObjectPtr<UPrimitiveComponent>> LevelPrimitiveComponents;	// 액터의 하위 컴포넌트는 액터에서 관리&해제됨

	FFrustumCull* Frustum = nullptr;

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	TObjectPtr<AActor> SelectedActor = nullptr;

	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
		static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
		static_cast<uint64>(EEngineShowFlags::SF_Bounds);

	// LOD Update System
	int32 LODUpdateFrameCounter = 0;
	static constexpr int32 LOD_UPDATE_INTERVAL = 10;  // 10프레임마다 업데이트

	/**
	 * @brief Level에서 Actor를 실질적으로 제거하는 함수
	 * 이전 Tick에서 마킹된 Actor를 제거한다
	 */
	void ProcessPendingDeletions();
};
