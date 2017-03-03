#include "Precompiled.h"
#include "Platform/PlatformProcess.h"
#include <unistd.h>
#include <dlfcn.h>

BE_NAMESPACE_BEGIN

SharedLib PlatformPosixProcess::OpenLibrary(const char *filename) {
	char cwd[1024];
	char path[1024];

	getcwd(cwd, sizeof(cwd));
	size_t len = strlen(cwd);
	if (cwd[len - 1] == '/') {
		cwd[len - 1] = 0;
	}

	snprintf(path, sizeof(path), "%s/%s", cwd, filename);
	void *handle = dlopen(path, RTLD_NOW);
	if (!handle) {
		char str[512];
#if defined(__MACOSX__)
		snprintf(str, sizeof(str), "%s.dylib", path);
#else
		snprintf(str, sizeof(str), "%s.so", path);
#endif
		handle = dlopen(str, RTLD_NOW);
	}

	return reinterpret_cast<SharedLib>(handle);
}

void *PlatformPosixProcess::GetSymbol(SharedLib lib, const char *sym) {
	return dlsym(lib, sym);
}

void PlatformPosixProcess::CloseLibrary(SharedLib lib) {
	if (lib) {
		dlclose((void *)lib);
	}
}

void PlatformPosixProcess::Sleep(float seconds) {
    const uint32_t usec = seconds * 1000000.0f;
    if (usec > 0) {
        usleep(usec);
    } else {
        sched_yield();
    }
}

BE_NAMESPACE_END
