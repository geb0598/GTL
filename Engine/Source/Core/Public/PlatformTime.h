#pragma once
#include "Global/Types.h"

#ifdef _DEBUG
#define TIME_PROFILE(Key)\
FScopeCycleCounter Key##Counter(#Key);
#else
#define TIME_PROFILE(Key)
#endif

#ifdef _DEVELOP
#define TIME_PROFILE_END(Key)\
Key##Counter.Finish();
#else
#define TIME_PROFILE_END(Key)
#endif

class FWindowsPlatformTime
{
public:
	static double GSecondsPerCycle; // 0
	static bool bInitialized; // false

	static void InitTiming()
	{
		if (!bInitialized)
		{
			bInitialized = true;

			double Frequency = (double)GetFrequency();
			if (Frequency <= 0.0)
			{
				Frequency = 1.0;
			}

			GSecondsPerCycle = 1.0 / Frequency;
		}
	}
	static float GetSecondsPerCycle()
	{
		if (!bInitialized)
		{
			InitTiming();
		}
		return (float)GSecondsPerCycle;
	}
	static uint64 GetFrequency()
	{
		LARGE_INTEGER Frequency;
		QueryPerformanceFrequency(&Frequency);
		return Frequency.QuadPart;
	}
	static double ToMilliseconds(uint64 CycleDiff)
	{
		double Ms = static_cast<double>(CycleDiff)
			* GetSecondsPerCycle()
			* 1000.0;

		return Ms;
	}

	static uint64 Cycles64()
	{
		LARGE_INTEGER CycleCount;
		QueryPerformanceCounter(&CycleCount);
		return (uint64)CycleCount.QuadPart;
	}
};
