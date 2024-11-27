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

class D3D12Buffer;

class D3D12IndexBuffer {
public:
    enum class Type : byte {
        Static = 0,
        Dynamic = 1
    };

    ~D3D12IndexBuffer() { Release(); }

    void                            Release();

    static D3D12IndexBuffer *       CreateIndexBuffer(Type type, int indexSize, int numIndexes, void *data);

    D3D12Buffer *                   buffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW         ibv = {};
};
