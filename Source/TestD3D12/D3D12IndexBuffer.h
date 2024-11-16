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

class D3D12IndexBuffer {
public:
    ~D3D12IndexBuffer() { Release(); }

    void                            Release();

    static D3D12IndexBuffer*        CreateIndexBuffer(int indexSize, int numIndexes, void *data);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation*            indexBufferAllocation = nullptr;
#else
    ID3D12Resource*                 indexBufferResource = nullptr;
#endif
    D3D12_INDEX_BUFFER_VIEW         ibv;
};
