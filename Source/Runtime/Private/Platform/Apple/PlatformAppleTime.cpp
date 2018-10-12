// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
