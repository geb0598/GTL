#include "pch.h"
#include "Core/Public/ScopeCycleCounter.h"

TMap<FString, FTimeProfile> TimeProfileMap;
void FScopeCycleCounter::AddTimeProfile(const TStatId StatId, const double Milliseconds)
{	if (TimeProfileMap.find(StatId.Key) != TimeProfileMap.end())
	{
		
	}
}

void FScopeCycleCounter::TimeProfileInit()
{
}

const TArray<FString> FScopeCycleCounter::GetTimeProfileKeys()
{
}

const TArray<FTimeProfile> FScopeCycleCounter::GetTimeProfileValues()
{
}

const FTimeProfile& FScopeCycleCounter::GetTimeProfile(const FString& Key)
{
}
