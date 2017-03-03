#pragma once

#define USE_BASE_PLATFORM_POSIX_PROCESS
#include "../Posix/PlatformPosixProcess.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformLinuxProcess : public PlatformPosixProcess {
public:
    static const wchar_t *      ExecutableFileName();
	static const wchar_t *		ComputerName();
	static const wchar_t *		UserName();
};

#ifndef USE_BASE_PLATFORM_LINUX_PROCESS
typedef PlatformLinuxProcess    PlatformProcess;
#endif

BE_NAMESPACE_END