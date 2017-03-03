#include "Precompiled.h"
#include "Platform/PlatformTime.h"
#include <mach/mach_time.h>

BE_NAMESPACE_BEGIN

void PlatformAppleTime::Init() {
	// Time base is in nano seconds.
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	secondsPerCycle = 1e-9 * (double)info.numer / (double)info.denom;
}

float PlatformAppleTime::Seconds() {
	uint64_t cycles = mach_absolute_time();
	// Add big number to make bugs apparent where return value is being passed to float
	return cycles * secondsPerCycle + 16777216.0f;
}

uint64_t PlatformAppleTime::Cycles() {
	uint64_t cycles = mach_absolute_time();
	return cycles;
}

BE_NAMESPACE_END
