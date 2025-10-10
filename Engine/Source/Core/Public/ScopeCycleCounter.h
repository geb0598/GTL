#pragma once
#include "Core/Public/PlatformTime.h"

struct TStatId
{
	FString Key;
	TStatId() = default;
	TStatId(const FString& InKey) : Key(InKey) {}
};
struct FTimeProfile
{
	double Milliseconds;
	uint32 CallCount;

	const FString GetConstChar() const
	{
		char buffer[64]; // static으로 해야 반환 가능
		snprintf(buffer, sizeof(buffer), " : %.3fms, Call : %d", Milliseconds, CallCount);
		return FString(buffer);
	}
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
	FScopeCycleCounter()
		: StartCycles(FPlatformTime::Cycles64())
		, UsedStatId(TStatId{})
	{
	}

	FScopeCycleCounter(TStatId StatId)
		: StartCycles(FPlatformTime::Cycles64())
		, UsedStatId(StatId)
	{
	}

	~FScopeCycleCounter()
	{
		Finish();
	}

	double Finish()
	{
		if (bIsFinish == true)
		{
			return 0;
		}
		bIsFinish = true;
		const uint64 EndCycles = FPlatformTime::Cycles64();
		const uint64 CycleDiff = EndCycles - StartCycles;

		double Milliseconds = FWindowsPlatformTime::ToMilliseconds(CycleDiff);
		if (UsedStatId.Key.empty() == false)
		{
			AddTimeProfile(UsedStatId, Milliseconds); //키 값이 있을경우 Map에 저장
		}
		return Milliseconds;
	}

	static void AddTimeProfile(const TStatId StatId, const double Milliseconds);
	static void TimeProfileInit();

	static const TArray<FString> GetTimeProfileKeys();
	static const TArray<FTimeProfile> GetTimeProfileValues();
	static const FTimeProfile& GetTimeProfile(const FString& Key);

private:
	bool bIsFinish = false;
	uint64 StartCycles;
	TStatId UsedStatId;
};
