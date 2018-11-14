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
#include "Platform/Windows/PlatformWinTLS.h"

BE_NAMESPACE_BEGIN

uint32_t PlatformWinTLS::AllocTlsSlot() {
    static_assert(TLS_OUT_OF_INDEXES == 0xFFFFFFFF, "TLS_OUT_OF_INDEXES == 0xFFFFFFFF");
    return ::TlsAlloc();
}

void PlatformWinTLS::FreeTlsSlot(uint32_t slotIndex) {
    ::TlsFree(slotIndex);
}

void PlatformWinTLS::SetTlsValue(uint32_t slotIndex, void *value) {
    ::TlsSetValue(slotIndex, value);
}

void *PlatformWinTLS::GetTlsValue(uint32_t slotIndex) {
    return ::TlsGetValue(slotIndex);
}

BE_NAMESPACE_END
