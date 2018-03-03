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
#include "PlatformMacOS.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

PlatformMacOS::PlatformMacOS() {
    window = nil;
}

void PlatformMacOS::Init() {
    PlatformGeneric::Init();
    
    cursorLocked = false;
}

void PlatformMacOS::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformMacOS::EnableMouse(bool enable) {
    PlatformGeneric::EnableMouse(enable);
}

void PlatformMacOS::SetMainWindowHandle(void *windowHandle) {
    window = (__bridge NSWindow *)windowHandle;
}

void PlatformMacOS::Quit() {
    [NSApp terminate: nil];
}

void PlatformMacOS::Log(const wchar_t *msg) {
    NSString *nsmsg = (__bridge NSString *)WideStringToCFString(msg);
    NSLog(@"%@", nsmsg);
}

void PlatformMacOS::Error(const wchar_t *msg) {
    NSString *nsmsg = (__bridge NSString *)WideStringToCFString(msg);
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:"Error"]];
    [alert setInformativeText:nsmsg];
    [alert setAlertStyle:NSAlertStyleCritical];
    [alert runModal];
    
    Quit();
}

bool PlatformMacOS::IsCursorLocked() const {
    return cursorLocked;
}

bool PlatformMacOS::LockCursor(bool lock) {
    if (!window | !mouseEnabled) {
        return false;
    }
    
    bool oldLocked = cursorLocked;

    if (lock ^ cursorLocked) {
        cursorLocked = lock;

        if (lock) {
            // Disconnects the mouse and cursor while an application is in the foreground.
            CGEventErr err = CGAssociateMouseAndMouseCursorPosition(false);
            if (err != CGEventNoErr) {
                BE_ERRLOG(L"Could not disable mouse movement, CGAssociateMouseAndMouseCursorPosition returned %d\n", err);
            }
            
            CGDisplayHideCursor(kCGDirectMainDisplay);
        } else {
            // Connect the mouse and cursor
            CGEventErr err = CGAssociateMouseAndMouseCursorPosition(true);
            if (err != CGEventNoErr) {
                BE_ERRLOG(L"Could not reenable mouse movement, CGAssociateMouseAndMouseCursorPosition returned %d\n", err);
            }
        
            CGDisplayShowCursor(kCGDirectMainDisplay);
        }
    }
    
    return oldLocked;
}

void PlatformMacOS::GetMousePos(Point &pos) const {
    CGEventRef ourEvent = CGEventCreate(NULL);
    CGPoint mouseLocation = CGEventGetLocation(ourEvent);
    CFRelease(ourEvent);

    CGPoint windowOrigin = [window frame].origin;
    mouseLocation.x = mouseLocation.x - windowOrigin.x;
    mouseLocation.y = mouseLocation.y - windowOrigin.y;
    pos.Set(mouseLocation.x, mouseLocation.y);
}

void PlatformMacOS::GenerateMouseDeltaEvent() {
    if (!window || !mouseEnabled) {
        return;
    }
    
    int32_t dx, dy;
    CGGetLastMouseDelta(&dx, &dy);

    QueEvent(Platform::MouseDeltaEvent, dx, dy, 0, NULL);
}

BE_NAMESPACE_END
