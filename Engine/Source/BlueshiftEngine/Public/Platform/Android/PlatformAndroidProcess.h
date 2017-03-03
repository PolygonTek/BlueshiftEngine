#pragma once
#define USE_BASE_PLATFORM_LINUX_PROCESS
#include "../Linux/PlatformLinuxProcess.h"

BE_NAMESPACE_BEGIN

struct ProcessHandle {
    ProcessHandle(void *task = NULL) { }
    bool IsValid() const { return false; }
    void Close();
};

class BE_API PlatformAndroidProcess : public PlatformLinuxProcess {
public:
	static const wchar_t *      ExecutableFileName();

	static const wchar_t *		ComputerName();
	static const wchar_t *		UserName();
};

typedef PlatformAndroidProcess      PlatformProcess;

BE_NAMESPACE_END