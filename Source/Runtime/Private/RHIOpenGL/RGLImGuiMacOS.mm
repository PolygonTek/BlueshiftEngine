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
#include "Platform/PlatformTime.h"
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include "Profiler/Profiler.h"

// Undocumented methods for creating cursors.
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

BE_NAMESPACE_BEGIN

static NSCursor*      g_MouseCursors[ImGuiMouseCursor_COUNT] = {};
static bool           g_MouseCursorHidden = false;

static void ImGui_ImplOSX_UpdateMouseCursor() {
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        if (!g_MouseCursorHidden) {
            g_MouseCursorHidden = true;
            [NSCursor hide];
        }
    } else {
        // Show OS mouse cursor
        [g_MouseCursors[g_MouseCursors[imgui_cursor] ? imgui_cursor : ImGuiMouseCursor_Arrow] set];
        if (g_MouseCursorHidden) {
            g_MouseCursorHidden = false;
            [NSCursor unhide];
        }
    }
}

static int mapCharacterToKey(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    if (c == 25) { // SHIFT+TAB -> TAB
        return 9;
    }
    if (c >= 0 && c < 256) {
        return c;
    }
    if (c >= 0xF700 && c < 0xF700 + 256) {
        return c - 0xF700 + 256;
    }
    return -1;
}

static void resetKeys() {
    ImGuiIO &io = ImGui::GetIO();
    for (int n = 0; n < IM_ARRAYSIZE(io.KeysDown); n++) {
        io.KeysDown[n] = false;
    }
}

bool ImGui_ImplOSX_HandleEvent(NSEvent *event, NSView *view) {
    ImGuiIO &io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeOtherMouseDown) {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
            io.MouseDown[button] = true;
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp || event.type == NSEventTypeOtherMouseUp) {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
            io.MouseDown[button] = false;
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged) {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [view convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint(mousePoint.x, view.bounds.size.height - mousePoint.y);
        io.MousePos = ImVec2(mousePoint.x, mousePoint.y);
    }

    if (event.type == NSEventTypeScrollWheel) {
        double wheel_dx = 0.0;
        double wheel_dy = 0.0;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
        if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6) {
            wheel_dx = [event scrollingDeltaX];
            wheel_dy = [event scrollingDeltaY];
            if ([event hasPreciseScrollingDeltas]) {
                wheel_dx *= 0.1;
                wheel_dy *= 0.1;
            }
        } else
#endif // MAC_OS_X_VERSION_MAX_ALLOWED
        {
            wheel_dx = [event deltaX];
            wheel_dy = [event deltaY];
        }

        if (fabs(wheel_dx) > 0.0) {
            io.MouseWheelH += wheel_dx * 0.1f;
        }
        if (fabs(wheel_dy) > 0.0) {
            io.MouseWheel += wheel_dy * 0.1f;
        }
        return io.WantCaptureMouse;
    }

    // FIXME: All the key handling is wrong and broken. Refer to GLFW's cocoa_init.mm and cocoa_window.mm.
    if (event.type == NSEventTypeKeyDown) {
        NSString *str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++) {
            int c = [str characterAtIndex:i];
            if (!io.KeyCtrl && !(c >= 0xF700 && c <= 0xFFFF) && c != 127) {
                io.AddInputCharacter((unsigned int)c);
            }

            // We must reset in case we're pressing a sequence of special keys while keeping the command pressed
            int key = mapCharacterToKey(c);
            if (key != -1 && key < 256 && !io.KeyCtrl) {
                resetKeys();
            }
            if (key != -1) {
                io.KeysDown[key] = true;
            }
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeKeyUp) {
        NSString *str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++) {
            int c = [str characterAtIndex:i];
            int key = mapCharacterToKey(c);
            if (key != -1) {
                io.KeysDown[key] = false;
            }
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged) {
        ImGuiIO &io = ImGui::GetIO();
        unsigned int flags = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        bool oldKeyCtrl = io.KeyCtrl;
        bool oldKeyShift = io.KeyShift;
        bool oldKeyAlt = io.KeyAlt;
        bool oldKeySuper = io.KeySuper;

        io.KeyCtrl      = flags & NSEventModifierFlagControl;
        io.KeyShift     = flags & NSEventModifierFlagShift;
        io.KeyAlt       = flags & NSEventModifierFlagOption;
        io.KeySuper     = flags & NSEventModifierFlagCommand;

        // We must reset them as we will not receive any keyUp event if they where pressed with a modifier
        if ((oldKeyShift && !io.KeyShift) || (oldKeyCtrl && !io.KeyCtrl) || (oldKeyAlt && !io.KeyAlt) || (oldKeySuper && !io.KeySuper)) {
            resetKeys();
        }
        return io.WantCaptureKeyboard;
    }

    return false;
}

void OpenGLRHI::ImGuiCreateContext(GLContext *ctx) {
    // Setup Dear ImGui context.
    ctx->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    ctx->imGuiLastTime = PlatformTime::Seconds();
    
    io.IniFilename = nullptr;

    // Setup back-end capabilities flags.
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "OpenGLRHI-MacOS";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeyDown[] array.
    const int offset_for_function_keys = 256 - 0xF700;
    io.KeyMap[ImGuiKey_Tab]             = '\t';
    io.KeyMap[ImGuiKey_LeftArrow]       = NSLeftArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_RightArrow]      = NSRightArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_UpArrow]         = NSUpArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_DownArrow]       = NSDownArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageUp]          = NSPageUpFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageDown]        = NSPageDownFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Home]            = NSHomeFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_End]             = NSEndFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Insert]          = NSInsertFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Delete]          = NSDeleteFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Backspace]       = 127;
    io.KeyMap[ImGuiKey_Space]           = 32;
    io.KeyMap[ImGuiKey_Enter]           = 13;
    io.KeyMap[ImGuiKey_Escape]          = 27;
    io.KeyMap[ImGuiKey_KeyPadEnter]     = 13;
    io.KeyMap[ImGuiKey_A]               = 'A';
    io.KeyMap[ImGuiKey_C]               = 'C';
    io.KeyMap[ImGuiKey_V]               = 'V';
    io.KeyMap[ImGuiKey_X]               = 'X';
    io.KeyMap[ImGuiKey_Y]               = 'Y';
    io.KeyMap[ImGuiKey_Z]               = 'Z';

    // Load cursors. Some of them are undocumented.
    g_MouseCursorHidden = false;
    g_MouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
    g_MouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
    g_MouseCursors[ImGuiMouseCursor_NotAllowed] = [NSCursor operationNotAllowedCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = [NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)] ? [NSCursor _windowResizeNorthSouthCursor] : [NSCursor resizeUpDownCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = [NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)] ? [NSCursor _windowResizeEastWestCursor] : [NSCursor resizeLeftRightCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = [NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] ? [NSCursor _windowResizeNorthEastSouthWestCursor] : [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = [NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] ? [NSCursor _windowResizeNorthWestSouthEastCursor] : [NSCursor closedHandCursor];

    // Note that imgui.cpp also include default OSX clipboard handlers which can be enabled
    // by adding '#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS' in imconfig.h and adding '-framework ApplicationServices' to your linker command-line.
    // Since we are already in ObjC land here, it is easy for us to add a clipboard handler using the NSPasteboard api.
    io.SetClipboardTextFn = [](void *, const char *str) -> void {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
    };

    io.GetClipboardTextFn = [](void *) -> const char * {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        NSString *available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
        if (![available isEqualToString:NSPasteboardTypeString]) {
            return NULL;
        }

        NSString *string = [pasteboard stringForType:NSPasteboardTypeString];
        if (string == nil) {
            return NULL;
        }

        const char *string_c = (const char*)[string UTF8String];
        size_t string_len = strlen(string_c);
        static ImVector<char> s_clipboard;
        s_clipboard.resize((int)string_len + 1);
        strcpy(s_clipboard.Data, string_c);
        return s_clipboard.Data;
    };

    ImGui_ImplOpenGL_Init("#version 150");
}

void OpenGLRHI::ImGuiDestroyContext(GLContext *ctx) {
    ImGui_ImplOpenGL_Shutdown();

    ImGui::DestroyContext(ctx->imGuiContext);
}

void OpenGLRHI::ImGuiBeginFrame(Handle ctxHandle) {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiBeginFrame");

    ImGui_ImplOpenGL_ValidateFrame();

    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    DisplayMetrics dm;
    GetDisplayMetrics(ctxHandle, &dm);

    // Setup display size
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dm.screenWidth, dm.screenHeight);
    io.DisplayFramebufferScale = ImVec2((float)dm.backingWidth / dm.screenWidth, (float)dm.backingHeight / dm.screenHeight);

    // Setup time step
    double currentTime = PlatformTime::Seconds();
    io.DeltaTime = currentTime - ctx->imGuiLastTime;
    ctx->imGuiLastTime = currentTime;

    ImGui_ImplOSX_UpdateMouseCursor();
    
    ImGui::NewFrame();
}

void OpenGLRHI::ImGuiRender() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiRender");
    BE_SCOPE_PROFILE_GPU("OpenGLRHI::ImGuiRender");

    ImGui::Render();

    bool sRGBWriteEnabled = OpenGL::SupportsFrameBufferSRGB() && IsSRGBWriteEnabled();
    if (sRGBWriteEnabled) {
        SetSRGBWrite(false);
    }

    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());

    if (sRGBWriteEnabled) {
        SetSRGBWrite(true);
    }
}

void OpenGLRHI::ImGuiEndFrame() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiEndFrame");

    ImGui::EndFrame();
}

BE_NAMESPACE_END
