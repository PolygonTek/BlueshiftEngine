#pragma once

#define USE_BASE_PLATFORM_POSIX_TIME
#include "../Posix/PlatformPosixTime.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformLinuxTime : public PlatformPosixTime {
public:
	static void             Init();

	static float            Seconds();

	static uint64_t         Cycles();
};

typedef PlatformLinuxTime	PlatformTime;

BE_NAMESPACE_END
