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
#include "Platform/PlatformTLS.h"
#include "Platform/Posix/PlatformPosixTLS.h"

BE_NAMESPACE_BEGIN

uint32_t PlatformPosixTLS::AllocTlsSlot() {
    // allocate a per-thread mem slot
    pthread_key_t key = 0;
    if (pthread_key_create(&key, nullptr) != 0) {
        key = 0xFFFFFFFF;
    }
    return (uint32_t)key;
}

void PlatformPosixTLS::FreeTlsSlot(uint32_t slotIndex) {
    pthread_key_delete((pthread_key_t)slotIndex);
}

void PlatformPosixTLS::SetTlsValue(uint32_t slotIndex, void *value) {
    pthread_setspecific((pthread_key_t)slotIndex, value);
}

void *PlatformPosixTLS::GetTlsValue(uint32_t slotIndex) {
    return pthread_getspecific((pthread_key_t)slotIndex);
}

BE_NAMESPACE_END
