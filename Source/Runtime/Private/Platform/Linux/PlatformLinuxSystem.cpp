// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformSystem.h"

bool PlatformLinuxSystem::IsDebuggerPresent() {
    const int status_fd = ::open("/proc/self/status", O_RDONLY);
    if (status_fd == -1) {
        return false;
    }

    char buf[4096];
    const ssize_t num_read = ::read(status_fd, buf, sizeof(buf) - 1);
    if (num_read <= 0) {
        return false;
    }

    bool isDebugging = false;
    const char *tracerPidString = "TracerPid:\t";
    const ssize_t tracerPidStringLen = strlen(tracerPidString);
    int i = 0;

    while ((num_read - i) > tracerPidStringLen) {
        // TracerPid is found
        if (strncmp(&buf[i], tracerPidString, tracerPidStringLen) == 0) {
            // 0 if no process is tracing.
            isDebugging = buf[i + tracerPidStringLen] != '0';
            break;
        }

        ++i;
    }

    ::close(status_fd);
    return isDebugging;
}

bool PlatformLinuxSystem::DebugBreak() {
    if (IsDebuggerPresent()) {
        raise(SIGTRAP);
    }
}
