#include "Precompiled.h"
#include "Platform/PlatformTime.h"
//#include <mach/mach_time.h>

BE_NAMESPACE_BEGIN

void PlatformLinuxTime::Init() {
	//// Time base is in nano seconds.
	//mach_timebase_info_data_t info;
	//mach_timebase_info(&info);
	//secondsPerCycle = 1e-9 * (double)info.numer / (double)info.denom;

}

float PlatformLinuxTime::Seconds() {
	//uint64_t cycles = mach_absolute_time();
	//// Add big number to make bugs apparent where return value is being passed to float
	//return cycles * secondsPerCycle + 16777216.0f;
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1 + tv.tv_usec * 0.000001f;
}

uint64_t PlatformLinuxTime::Cycles() {
	//uint64_t cycles = mach_absolute_time();
	//return cycles;
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000000L + tv.tv_usec;
}

BE_NAMESPACE_END
