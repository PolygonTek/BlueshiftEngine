#include "Precompiled.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

void PlatformLinuxTime::Init() {
}

double PlatformLinuxTime::Seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
}

uint64_t PlatformLinuxTime::Cycles() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000ULL + static_cast<uint64_t>(ts.tv_nsec) / 1000ULL;
}

BE_NAMESPACE_END
