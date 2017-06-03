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

#include "Platform/PlatformAtomic.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

class Texture;

struct BufferCache {
    RHI::Handle             buffer;             // buffer handle (dynamic or static)
    const Texture *         texture;
    uint32_t                tcBase[2];
    uint32_t                offset;             // offset for dynamic buffer. always 0 for static buffer.
    uint32_t                bytes;              // 
    uint32_t                frameCount;         // always 0xFFFFFFFF for static buffer
};

class BufferCacheManager {
public:
    void                    Init();
    void                    Shutdown();

    void                    BeginWrite();
    void                    EndDrawCommand();

    void                    BeginBackEnd();

                            /// Allocates vertex buffer in the local device memory
    void                    AllocStaticVertex(int bytes, const void *data, BufferCache *vc);
                            /// Allocates index buffer in the local device memory
    void                    AllocStaticIndex(int bytes, const void *data, BufferCache *vc);
                            /// Allocates texel buffer in the local device memory
    void                    AllocStaticTexel(int bytes, const void *data, BufferCache *vc);

    bool                    AllocVertex(int numVertexes, int vertexSize, const void *data, BufferCache *vc);
    bool                    AllocIndex(int numIndexes, int indexSize, const void *data, BufferCache *vc);
    bool                    AllocTexel(int bytes, const void *data, BufferCache *vc);

    byte *                  MapVertexBuffer(BufferCache *bc) const;
    byte *                  MapIndexBuffer(BufferCache *bc) const;
    byte *                  MapTexelBuffer(BufferCache *bc) const;

    void                    UnmapVertexBuffer(BufferCache *bc) const;
    void                    UnmapIndexBuffer(BufferCache *bc) const;
    void                    UnmapTexelBuffer(BufferCache *bc) const;

    bool                    IsCached(const BufferCache *vc) const;

                            // Update PBO to Texture
    void                    UpdatePBOTexture() const;

    const Texture *         GetFrameTexture() const;

    RHI::Handle             streamVertexBuffer;
    RHI::Handle             streamIndexBuffer;

private:
    static const int        NumFrames = 3;

    struct FrameDataBufferSet {
        RHI::Handle         vertexBuffer;
        RHI::Handle         indexBuffer;
        RHI::Handle         texelBuffer;
        RHI::BufferType     texelBufferType;
        Texture *           texture;
        RHI::Handle         sync;
        void *              mappedVertexBase;
        void *              mappedIndexBase;
        void *              mappedTexelBase;
        PlatformAtomic      vertexMemUsed;
        PlatformAtomic      indexMemUsed;
        PlatformAtomic      texelMemUsed;
        int                 allocations;
    };

    void                    MapBufferSet(FrameDataBufferSet &bufferSet);
    void                    UnmapBufferSet(FrameDataBufferSet &bufferSet);

    FrameDataBufferSet      frameData[NumFrames];
    uint32_t                frameCount;
    int                     mappedNum;
    int                     unmappedNum;

    int                     mostUsedVertexMem;
    int                     mostUsedIndexMem;
    int                     mostUsedTexelMem;

    int                     pboWriteOffset;
};

extern BufferCacheManager   bufferCacheManager;

BE_NAMESPACE_END
