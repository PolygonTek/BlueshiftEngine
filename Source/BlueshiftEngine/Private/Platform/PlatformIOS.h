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

#pragma once

#include "PlatformGeneric.h"
//#include <CoreGraphics/CoreGraphics.h>

OBJC_CLASS(UIWindow);

BE_NAMESPACE_BEGIN

class PlatformIOS : public PlatformGeneric {
public:
    PlatformIOS();
    
    virtual void            Init() override;
    virtual void            Shutdown() override;

    virtual void            SetMainWindowHandle(void *windowHandle) override;

    virtual void            Quit() override;
    virtual void            Log(const wchar_t *msg) override;
    virtual void            Error(const wchar_t *msg) override;

private:
    UIWindow *              window;
};

BE_NAMESPACE_END
