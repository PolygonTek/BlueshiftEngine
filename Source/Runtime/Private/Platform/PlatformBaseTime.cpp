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

BE_NAMESPACE_BEGIN

double PlatformBaseTime::secondsPerCycle;

void PlatformBaseTime::Init() {
    secondsPerCycle = 0;
}

void PlatformBaseTime::Shutdown() {
}

float PlatformBaseTime::Seconds() {
    return 0.0f;
}

uint32_t PlatformBaseTime::Milliseconds() {
    return 0;
}

uint64_t PlatformBaseTime::Microseconds() {
    return 0;
}

uint64_t PlatformBaseTime::Cycles() {
    return 0;
}

int PlatformBaseTime::GetTimeOfDay(struct timeval *tv) {
    return 0;
}

void PlatformBaseTime::LocalTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
}

void PlatformBaseTime::UtcTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
}

BE_NAMESPACE_END
