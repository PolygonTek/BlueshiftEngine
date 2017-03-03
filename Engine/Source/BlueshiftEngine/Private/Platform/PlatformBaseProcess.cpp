#include "Precompiled.h"
#include "Core/WStr.h"
#include "Platform/PlatformProcess.h"

BE_NAMESPACE_BEGIN

const char *PlatformBaseProcess::PlatformName() {
#if defined(__LINUX__) && !defined(__X86_64__)
    return "Linux-x86";
#elif defined(__LINUX__) && defined(__X86_64__)
    return "Linux-x64";
#elif defined(__FREEBSD__) && !defined(__X86_64__)
    return "FreeBSD-x86";
#elif defined(__FREEBSD__) && defined(__X86_64__)
    return "FreeBSD-x64";
#elif defined(__CYGWIN__) && !defined(__X86_64__)
    return "Cygwin-x86";
#elif defined(__CYGWIN__) && defined(__X86_64__)
    return "Cygwin-x64";
#elif defined(__WIN32__) && !defined(__X86_64__)
    return "Windows-x86";
#elif defined(__WIN32__) && defined(__X86_64__)
    return "Windows-x64";
#elif defined(__MACOSX__) && !defined(__X86_64__)
    return "MacOSX-x86";
#elif defined(__MACOSX__) && defined(__X86_64__)
    return "MacOSX-x64";
#elif defined(__UNIX__) && !defined(__X86_64__)
    return "Unix-x86";
#elif defined(__UNIX__) && defined(__X86_64__)
    return "Unix-x64";
#else
    return "Unknown";
#endif
}

SharedLib PlatformBaseProcess::OpenLibrary(const char *filename) {
    return nullptr;
}

void *PlatformBaseProcess::GetSymbol(SharedLib lib, const char *sym) {
    return nullptr;
}

void PlatformBaseProcess::CloseLibrary(SharedLib lib) {
}

const wchar_t *PlatformBaseProcess::ExecutableFileName() {
    return L"";
}

const wchar_t *PlatformBaseProcess::ComputerName() {
    return L"";
}

const wchar_t *PlatformBaseProcess::UserName() {
    return L"";
}

ProcessHandle PlatformBaseProcess::CreateProccess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath) {
    return ProcessHandle();
}

bool PlatformBaseProcess::IsProccessRunning(ProcessHandle &processHandle) {
    return false;
}

void PlatformBaseProcess::WaitForProccess(ProcessHandle &processHandle) {
}

void PlatformBaseProcess::TerminateProccess(ProcessHandle &processHandle) {
}

bool PlatformBaseProcess::GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode) {
    return false;
}

bool PlatformBaseProcess::ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer) {
    return false;
}

void PlatformBaseProcess::Sleep(float seconds) {
}

BE_NAMESPACE_END
