#pragma once

#define USE_BASE_PLATFORM_POSIX_PROCESS
#include "../Posix/PlatformPosixProcess.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformAppleProcess : public PlatformPosixProcess {
public:
    static const wchar_t *      ExecutableFileName();
	static const wchar_t *		ComputerName();
	static const wchar_t *		UserName();
};

#ifndef USE_BASE_PLATFORM_APPLE_PROCESS
typedef PlatformAppleProcess    PlatformProcess;
#endif

BE_NAMESPACE_END