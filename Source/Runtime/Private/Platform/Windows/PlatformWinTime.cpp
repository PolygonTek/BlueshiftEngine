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
#include "Platform/Windows/PlatformWinTime.h"
#include <mmsystem.h>
#include <winsock2.h>

BE_NAMESPACE_BEGIN

double PlatformWinTime::secondsPerCycle;

void PlatformWinTime::Init() {
    LARGE_INTEGER frequency;

    if (!QueryPerformanceFrequency(&frequency)) {
        BE_FATALERROR("No hardware timer available");
        return;
    }

    secondsPerCycle = 1.0 / frequency.QuadPart;

    PlatformTime::Seconds();
}

void PlatformWinTime::Shutdown() {
    timeEndPeriod(1);
}

double PlatformWinTime::Seconds() {
    LARGE_INTEGER cycles;

    QueryPerformanceCounter(&cycles);

    return cycles.QuadPart * secondsPerCycle;
}

uint32_t PlatformWinTime::Milliseconds() {
    return (uint32_t)(PlatformTime::Seconds() * 1e3);
}

uint64_t PlatformWinTime::Microseconds() {
    return (uint64_t)(PlatformTime::Seconds() * 1e6);
}

uint64_t PlatformWinTime::Nanoseconds() {
    return (uint64_t)(PlatformTime::Seconds() * 1e9);
}

uint64_t PlatformWinTime::Cycles() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}

int PlatformWinTime::GetTimeOfDay(struct timeval *tv) {
    static constexpr uint64_t delta_epoch_in_microsecs = 116444736000000000Ui64;
    uint64_t tmpres = 0;
    FILETIME ft;
    
    if (nullptr != tv) {
        GetSystemTimeAsFileTime(&ft);
 
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
 
        // converting file time to unix epoch
        tmpres -= delta_epoch_in_microsecs; 
        tmpres /= 10;  // convert into microseconds
        tv->tv_sec = (int32_t)(tmpres / 1000000);
        tv->tv_usec = (int32_t)(tmpres % 1000000);
    }
    
    return 0;
}

void PlatformWinTime::LocalTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    year        = st.wYear;
    month       = st.wMonth;
    dayOfWeek   = st.wDayOfWeek;
    day         = st.wDay;
    hour        = st.wHour;
    min         = st.wMinute;
    sec         = st.wSecond;
    msec        = st.wMilliseconds;
}

void PlatformWinTime::UtcTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec) {
    SYSTEMTIME st;
    GetSystemTime(&st);

    year        = st.wYear;
    month       = st.wMonth;
    dayOfWeek   = st.wDayOfWeek;
    day         = st.wDay;
    hour        = st.wHour;
    min         = st.wMinute;
    sec         = st.wSecond;
    msec        = st.wMilliseconds;
}

BE_NAMESPACE_END
