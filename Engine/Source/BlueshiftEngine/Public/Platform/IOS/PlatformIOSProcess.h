#pragma once

#define USE_BASE_PLATFORM_APPLE_PROCESS
#include "../Apple/PlatformAppleProcess.h"

BE_NAMESPACE_BEGIN

struct ProcessHandle {
    ProcessHandle(void *task = NULL) { }
    bool IsValid() const { return false; }
    void Close();
};

class BE_API PlatformIOSProcess : public PlatformAppleProcess {
public:
	static const wchar_t *		ComputerName();
	static const wchar_t *		UserName();
};

typedef PlatformIOSProcess      PlatformProcess;

BE_NAMESPACE_END