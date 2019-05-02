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

BE_NAMESPACE_BEGIN

class BE_API PlatformBaseTLS {
public:
    static uint32_t             AllocTlsSlot();
    static void                 FreeTlsSlot(uint32_t slotIndex);

    static void                 SetTlsValue(uint32_t slotIndex, void *value);
    static void *               GetTlsValue(uint32_t slotIndex);
};

BE_NAMESPACE_END

#if defined(__UNIX__) 
#include "Posix/PlatformPosixTLS.h"
#elif defined(__WIN32__)
#include "Windows/PlatformWinTLS.h"
#endif
