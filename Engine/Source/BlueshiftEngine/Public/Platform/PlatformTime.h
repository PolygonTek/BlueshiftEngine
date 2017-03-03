#pragma once

BE_NAMESPACE_BEGIN

class BE_API PlatformBaseTime {
public:
    static void             Init();
    static void             Shutdown();

    static float            Seconds();
    static uint32_t         Milliseconds();
    static uint64_t         Microseconds();

    static uint64_t         Cycles();
    static double           SecondsPerCycles() { return secondsPerCycle; }

    static int              GetTimeOfDay(struct timeval *tv);
    static void             LocalTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec);
    static void             UtcTime(int32_t &year, int32_t &month, int32_t &dayOfWeek, int32_t &day, int32_t &hour, int32_t &min, int32_t &sec, int32_t &msec);

protected:
    static double           secondsPerCycle;
};

BE_NAMESPACE_END

#ifdef __WIN32__
#include "Windows/PlatformWinTime.h"
#elif defined(__APPLE__)
#include "Apple/PlatformAppleTime.h"
#elif defined(__LINUX__)
#include "Linux/PlatformLinuxTime.h"
#elif defined(__UNIX__)
#include "Posix/PlatformPosixTime.h"
#endif
