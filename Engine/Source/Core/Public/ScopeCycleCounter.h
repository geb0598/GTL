#pragma once
#include "Core/Public/PlatformTime.h"

struct TStatId
{
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

	uint64 Finish()
	{
		const uint64 EndCycles = FPlatformTime::Cycles64();
		const uint64 CycleDiff = EndCycles - StartCycles;

		// FThreadStats::AddMessage(UsedStatId, EStatOperation::Add, CycleDiff);

		return FPlatformTime::ToMilliseconds(CycleDiff); // ms 변환 후 리턴
	}

private:
	uint64 StartCycles;
	TStatId UsedStatId;
};
