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
#ifdef __WIN32__
#include "PlatformWin.h"
#elif defined __MACOSX__
#include "PlatformMacOS.h"
#elif defined __IOS__
#include "PlatformIOS.h"
#elif defined __ANDROID__
#include "PlatformAndroid.h"
#endif

BE_NAMESPACE_BEGIN

PlatformAbstract *  platform = nullptr;

void Platform::Init() {
#ifdef __WIN32__
    platform = new PlatformWin;
#elif defined __MACOSX__
    platform = new PlatformMacOS;
#elif defined __IOS__
    platform = new PlatformIOS;
#elif defined __ANDROID__
    platform = new PlatformAndroid;
#endif

    platform->Init();
}

void Platform::Shutdown() {
    platform->Shutdown();

    delete platform;
}

PlatformAbstract::~PlatformAbstract() {
}

BE_NAMESPACE_END
