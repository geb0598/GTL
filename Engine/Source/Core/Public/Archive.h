#pragma once

#include <type_traits>

#include "Global/CoreTypes.h"
#include "Global/Vector.h"

struct FArchive
{
	virtual ~FArchive() = default;

	/** Returns true if this archive is for loading data. */
	virtual bool IsLoading() const = 0;
	virtual void Serialize(void* V, size_t Length) = 0;

	template<typename T>
	FArchive& operator<<(T& Value)
	{
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			Serialize(&Value, sizeof(T));
		}
		else
		{
			UE_LOG_ERROR("직렬화 할 수 없는 형식입니다.");
		}
		return *this;
	}

	template<typename T>
	FArchive& operator<<(TArray<T>& Value)
	{
		size_t Length = Value.size();
		if (!IsLoading())
		{
			*this << Length;
		}
		else
		{
			*this << Length;
			Value.resize(Length);
		}

		for (T& Element : Value)
		{
			*this << Element;
		}
		return *this;
	}

	FArchive& operator<<(FString& Value)
	{
		size_t Length = Value.size();
		if (!IsLoading())
		{
			*this << Length;
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}
		else
		{
			*this << Length;
			Value.resize(Length);
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}
		return *this;
	}
};
