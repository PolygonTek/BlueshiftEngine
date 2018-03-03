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
#include <sys/time.h>

BE_NAMESPACE_BEGIN

void PlatformPosixTime::Init() {
    // we use gettimeofday() instead of rdtsc, so it's 1000000 "cycles" per second on this faked CPU.
    secondsPerCycle = 1.0f / 1000000.0f;
}

void PlatformPosixTime::Shutdown() {
}

float PlatformPosixTime::Seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((double)tv.tv_sec) + (((double)tv.tv_usec) / 1000000.0);
}

uint32_t PlatformPosixTime::Milliseconds() {
    static unsigned int	starttime = 0;
    struct timeval tv;

    gettimeofday(&tv, NULL);
#if defined(__vms) && (__VMS_VER < 70000000)
    uint32_t now = tv.val / TICKS_PER_MILLISECOND;
#else
    uint32_t now = (uint32_t)tv.tv_sec * 1000 + (uint32_t)tv.tv_usec / 1000;
#endif

    if (!starttime) {
        starttime = now;
        return 0;
    }

    return now - starttime;
}

uint64_t PlatformPosixTime::Microseconds() {
    return (uint64_t)(PlatformPosixTime::Seconds() * 1000000.0f);
}

uint64_t PlatformPosixTime::Cycles() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec) * 1000000ULL) + (((uint64_t)tv.tv_usec));
}

int PlatformPosixTime::GetTimeOfDay(struct timeval *tv) {
    return gettimeofday(tv, NULL);
}

void PlatformPosixTime::LocalTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
    // query for calendar time
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // convert it to local time
    struct tm ltm;
    localtime_r(&tv.tv_sec, &ltm);

    // pull out data/time
    year		= ltm.tm_year + 1900;
    month		= ltm.tm_mon + 1;
    dayOfWeek	= ltm.tm_wday;
    day			= ltm.tm_mday;
    hour		= ltm.tm_hour;
    min			= ltm.tm_min;
    sec			= ltm.tm_sec;
    msec		= tv.tv_usec / 1000;
}

void PlatformPosixTime::UtcTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
    // query for calendar time
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // convert it to UTC
    struct tm ltm;
    gmtime_r(&tv.tv_sec, &ltm);

    // pull out data/time
    year		= ltm.tm_year + 1900;
    month		= ltm.tm_mon + 1;
    dayOfWeek	= ltm.tm_wday;
    day			= ltm.tm_mday;
    hour		= ltm.tm_hour;
    min			= ltm.tm_min;
    sec			= ltm.tm_sec;
    msec		= tv.tv_usec / 1000;
}

BE_NAMESPACE_END
