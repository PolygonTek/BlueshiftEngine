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
#include <pwd.h>
#include <unistd.h>

BE_NAMESPACE_BEGIN

const char *PlatformPosixSystem::UserDir() {
    static char path[1024] = "";

    if (!path[0]) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            strcpy(path, pwd->pw_dir);
        } else {
            // try the $HOME environment variable
            strcpy(path, getenv("HOME"));
        }
    }
    return path;
}

BE_NAMESPACE_END
