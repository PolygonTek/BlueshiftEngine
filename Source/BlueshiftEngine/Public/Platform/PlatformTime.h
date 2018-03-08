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

#pragma once

struct timeval;

BE_NAMESPACE_BEGIN

class BE_API PlatformBaseTime {
public:
    static void             Init();
    static void             Shutdown();

                            /// Returns time in seconds.
    static float            Seconds();

                            /// Returns time in milliseconds.
    static uint32_t         Milliseconds();

                            /// Returns time in microseconds.
    static uint64_t         Microseconds();

                            /// Returns CPU clocks counter.
    static uint64_t         Cycles();

    static double           SecondsPerCycles() { return secondsPerCycle; }

    static int              GetTimeOfDay(struct timeval *tv);
    static void             LocalTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec);
    static void             UtcTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec);

protected:
    static double           secondsPerCycle;
};

BE_NAMESPACE_END

#if defined(__APPLE__)
#include "Apple/PlatformAppleTime.h"
#elif defined(__LINUX__)
#include "Linux/PlatformLinuxTime.h"
#elif defined(__UNIX__)
#include "Posix/PlatformPosixTime.h"
#elif defined(__WIN32__)
#include "Windows/PlatformWinTime.h"
#endif
