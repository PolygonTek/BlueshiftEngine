#pragma once

#define USE_BASE_PLATFORM_POSIX_TIME
#include "../Posix/PlatformPosixTime.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformAppleTime : public PlatformPosixTime {
public:
	static void             Init();

	static float            Seconds();

	static uint64_t         Cycles();
};

typedef PlatformAppleTime	PlatformTime;

BE_NAMESPACE_END
