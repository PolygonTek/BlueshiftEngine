#include "Precompiled.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

void PlatformLinuxTime::Init() {
}

double PlatformLinuxTime::Seconds() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1 + tv.tv_usec * 0.000001;
}

uint64_t PlatformLinuxTime::Cycles() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

BE_NAMESPACE_END
