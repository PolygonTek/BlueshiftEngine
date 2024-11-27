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

#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

class D3D12Buffer {
public:
    enum class Usage : byte {
        Default = 0,
        Upload = 1,
        Readback = 2
    };

    ~D3D12Buffer() { Release(); }

    void                            Release();

    ID3D12Resource *                GetResource();
    UINT                            GetSize();

    static D3D12Buffer *            CreateBuffer(Usage usage, int size);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           bufferAllocation = nullptr;
#else
    ID3D12Resource *                bufferResource = nullptr;
#endif
};
