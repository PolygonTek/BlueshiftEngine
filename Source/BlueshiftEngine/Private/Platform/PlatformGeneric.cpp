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
#include "Core/Heap.h"
#include "Render/Render.h"
#include "Input/KeyCmd.h"
#include "Platform/PlatformTime.h"
#include "PlatformGeneric.h"

BE_NAMESPACE_BEGIN

PlatformGeneric::PlatformGeneric() {
}

void PlatformGeneric::Init() {
    active = false;
    minimized = false;
    mouseEnabled = false;

    numEvents = 0;
    numProcessedEvents = 0;
}

void PlatformGeneric::Shutdown() {
}

void PlatformGeneric::EnableMouse(bool enable) {
    mouseEnabled = enable;
}

void PlatformGeneric::GetEvent(Platform::Event *ev) {
    if (numEvents > numProcessedEvents) {
        memcpy(ev, &eventQueue[numProcessedEvents & (Platform::MaxPlatformEvents - 1)], sizeof(Platform::Event));
        numProcessedEvents++;
        return;
    }
    /*
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            Shutdown();
            exit(0);
        }
    }

    // TODO
    MSG_Init(&sb, buffer, sizeof(buffer));
    if (NetworkSystem::GetPacket(&adr, &sb)) {
        int size = sb.cursize - sb.readcount + sizeof(adr);
        byte *data = (byte *)Dynamic_Malloc(size);
        memcpy(data, &adr, sizeof(adr));
        memcpy(data+sizeof(adr), sb.data, sb.cursize-sb.readcount);
        QueSysEvent(0, PacketEvent, 0, 0, size, data);
    }*/

    if (numEvents > numProcessedEvents) {
        memcpy(ev, &eventQueue[numProcessedEvents & (Platform::MaxPlatformEvents - 1)], sizeof(Platform::Event));
        numProcessedEvents++;
        return;
    }

    // 받아올 event 가 없는 경우 NULL event
    memset(ev, 0, sizeof(Platform::Event));
    ev->time = PlatformTime::Milliseconds();
}

void PlatformGeneric::QueEvent(Platform::EventType type, int64_t value, int64_t value2, int ptrLength, void *ptr) {
    Platform::Event *ev = &eventQueue[numEvents & (Platform::MaxPlatformEvents - 1)];

    if (numEvents - numProcessedEvents == Platform::MaxPlatformEvents) {
        BE_LOG(L"PlatformGeneric::QueEvent: overflow\n");
        if (ev->ptr) {
            Mem_Free(ev->ptr);
        }

        numProcessedEvents++;
    }

    numEvents++;
    
    ev->time = PlatformTime::Milliseconds();
    ev->type = type;
    ev->value = value;
    ev->value2 = value2;
    ev->ptrLength = ptrLength;
    ev->ptr = ptr;
}

bool PlatformGeneric::IsActive() const {
    return active;
}

void PlatformGeneric::AppActivate(bool active, bool minimized) {
    this->minimized = minimized;

    keyCmdSystem.ClearStates();
    
    if (active && !minimized) {
        this->active = true;
        if (renderSystem.IsInitialized()) {
            renderSystem.SetGamma(cvarSystem.GetCVarFloat(L"r_gamma"));
        }
    } else {
        this->active = false;
        if (renderSystem.IsInitialized()) {
            renderSystem.RestoreGamma();
        }
    }
}

BE_NAMESPACE_END
