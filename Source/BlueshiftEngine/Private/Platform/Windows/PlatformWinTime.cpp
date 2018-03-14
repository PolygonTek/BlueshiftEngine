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

// 'HRT' - High resolution timer
static LARGE_INTEGER    hrt_frequency;
static double           hrt_secs;
static int              hrt_hz;
static int              hrt_shift;
static double           hrt_curtime = 0.0;
static int              hrt_sametimecount = 0;

void PlatformWinTime::Init() {
    unsigned int lpart;
    unsigned int hpart;

    if (!QueryPerformanceFrequency(&hrt_frequency)) {
        BE_FATALERROR(L"No hardware timer available");
        return;
    }

    lpart = (unsigned int)hrt_frequency.LowPart;
    hpart = (unsigned int)hrt_frequency.HighPart;

    hrt_shift = 0;

    // 1/2000000(=0.0000005) frequency 이하의 정밀도를 맞추기 위한 frequency shift 값을 산출한다
    while (hpart || lpart > 2000000) {
        hrt_shift++;

        lpart >>= 1;
        lpart |= (hpart & 1) << 31;
        hpart >>= 1;
    }

    hrt_hz = lpart;
    hrt_secs = 1.0 / (double)lpart;

    PlatformTime::Seconds();

    timeBeginPeriod(1);

    secondsPerCycle = 1.0 / hrt_frequency.QuadPart;
}

void PlatformWinTime::Shutdown() {
    timeEndPeriod(1);
}

float PlatformWinTime::Seconds() {
    static int first = 1;
    static int sametimecount;
    static unsigned int oldcount;
    unsigned int count, d;
    LARGE_INTEGER performanceCount;
    double time;
    double lasttime;

    QueryPerformanceCounter(&performanceCount);

    count = ((unsigned int)performanceCount.LowPart >> hrt_shift) | ((unsigned int)performanceCount.HighPart << (32 - hrt_shift));

    if (first) {
        oldcount = count;
        first = 0;
    } else {
        if (oldcount >= count && (oldcount - count) < 0x10000000) {
            oldcount = count;
        } else {
            d = count - oldcount;
            oldcount = count;

            time = (double)d * hrt_secs;
            lasttime = hrt_curtime;
            hrt_curtime += time;

            if (hrt_curtime == lasttime) {
                hrt_sametimecount++;

                if (hrt_sametimecount >= hrt_hz) {
                    hrt_curtime += 1.0;
                    sametimecount = 0;
                }
            } else {
                hrt_sametimecount = 0;
            }
        }
    }

    return (float)hrt_curtime;
}

uint32_t PlatformWinTime::Milliseconds() {
    static uint32_t starttime = 0;
    uint32_t now = timeGetTime();

    if (!starttime) {
        starttime = now;
        return 0;
    }

    return now - starttime;
}

uint64_t PlatformWinTime::Microseconds() {
    return (uint64_t)(PlatformTime::Seconds() * 1000000.0f);
}

uint64_t PlatformWinTime::Cycles() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
}

int PlatformWinTime::GetTimeOfDay(struct timeval *tv) {
    static const uint64_t delta_epoch_in_microsecs = 116444736000000000Ui64;
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
