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
#include "PlatformGeneric.h"
#include "PlatformAndroid.h"
#include <android/log.h>
#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

PlatformAndroid::PlatformAndroid() {
//    window = nil;
}

void PlatformAndroid::Init() {
    PlatformGeneric::Init();
}

void PlatformAndroid::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformAndroid::SetMainWindowHandle(void *windowHandle) {
//	window = (NvEGLUtil *)windowHandle;
}

void PlatformAndroid::Quit() {
    //[[UIApplication sharedApplication] terminateWithSuccess];
 //   exit(EXIT_SUCCESS);
    assert(0);;
}

void PlatformAndroid::Log(const wchar_t *msg) {
    __android_log_write(ANDROID_LOG_INFO, "Blueshift", tombs(msg));
}

void PlatformAndroid::Error(const wchar_t *msg) {
    __android_log_write(ANDROID_LOG_ERROR, "Blueshift", tombs(msg));

    Quit();
}

BE_NAMESPACE_END
