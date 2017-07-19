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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

#define PINNED_MEMORY   1
#define PERSISTENT_MAP  0

BufferCacheManager      bufferCacheManager;

static const int        TB_WIDTH    = 4096;
static const int        TB_HEIGHT   = 32;
static const int        TB_BPP      = 4 * sizeof(float); // size of float RGBA
static const int        TB_PITCH    = TB_BPP * TB_WIDTH;
static const int        TB_BYTES    = TB_PITCH * TB_HEIGHT;

void BufferCacheManager::Init() {
    int vertexBytes = r_dynamicCacheVertexBytes.GetInteger();
    int indexBytes = r_dynamicCacheIndexBytes.GetInteger();

    for (int i = 0; i < COUNT_OF(frameData); i++) {
        FrameDataBufferSet *bufferSet = &frameData[i];
        
        memset(bufferSet, 0, sizeof(frameData[0]));

        bufferSet->vertexBuffer = rhi.CreateBuffer(RHI::VertexBuffer, RHI::Dynamic, vertexBytes, 0, nullptr);
        bufferSet->indexBuffer = rhi.CreateBuffer(RHI::IndexBuffer, RHI::Dynamic, indexBytes, 0, nullptr);

        if (renderGlobal.vtUpdateMethod == Mesh::TboUpdate) {
            // Create texture buffer to write directly
            bufferSet->texelBufferType = RHI::TexelBuffer;
            bufferSet->texelBuffer = rhi.CreateBuffer(bufferSet->texelBufferType, RHI::Dynamic, TB_BYTES, 0, nullptr);
            bufferSet->texture = textureManager.AllocTexture(va("_tbTexture%i", i));
            bufferSet->texture->CreateFromBuffer(Image::RGBA_32F_32F_32F_32F, bufferSet->texelBuffer);
        } else if (renderGlobal.vtUpdateMethod == Mesh::PboUpdate) {
            // Create unpack buffer to translate data from PBO to VTF texture
            // See below link if you want to know what PBO is 
            // http://www.songho.ca/opengl/gl_pbo.html
            bufferSet->texelBufferType = RHI::PixelUnpackBuffer;
            bufferSet->texelBuffer = rhi.CreateBuffer(bufferSet->texelBufferType, RHI::Dynamic, TB_BYTES, TB_PITCH, nullptr);
            if (i == 0) {
                bufferSet->texture = textureManager.AllocTexture("_tbTexture");
                bufferSet->texture->CreateEmpty(RHI::Texture2D, TB_WIDTH, TB_HEIGHT, 1, 1, 1,
                    Image::RGBA_32F_32F_32F_32F, Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality | Texture::HighPriority);
            }
        }
    }
    
    BE_LOG(L"dynamic vertex buffer created (%hs x %i)\n", Str::FormatBytes(vertexBytes).c_str(), COUNT_OF(frameData));
    BE_LOG(L"dynamic index buffer created (%hs x %i)\n", Str::FormatBytes(indexBytes).c_str(), COUNT_OF(frameData));
    if (frameData[0].texelBuffer) {
        BE_LOG(L"dynamic texel buffer created (%hs x %i)\n", Str::FormatBytes(TB_BYTES).c_str(), COUNT_OF(frameData));
    }
    
    // Create stream buffer for use in debug drawing
    streamVertexBuffer = rhi.CreateBuffer(RHI::VertexBuffer, RHI::Stream, 0);
    streamIndexBuffer = rhi.CreateBuffer(RHI::IndexBuffer, RHI::Stream, 0);

    pboWriteOffset = 0;

    frameCount = 0;
    mappedNum = 0;
    unmappedNum = -1;

    mostUsedVertexMem = 0;
    mostUsedIndexMem = 0;
    mostUsedTexelMem = 0;

#if PINNED_MEMORY
    MapBufferSet(frameData[mappedNum]);
#if PERSISTENT_MAP
    for (int i = mappedNum + 1; i < COUNT_OF(frameData); i++) {
        MapBufferSet(frameData[i]);
    }
#endif
#endif
}

void BufferCacheManager::Shutdown() {
    rhi.DeleteBuffer(streamVertexBuffer);
    rhi.DeleteBuffer(streamIndexBuffer);

    for (int i = 0; i < COUNT_OF(frameData); i++) {
#if PINNED_MEMORY
        UnmapBufferSet(frameData[i]);

        if (frameData[i].sync != RHI::NullSync) {
            rhi.DeleteSync(frameData[i].sync);
        }
#endif

        rhi.DeleteBuffer(frameData[i].vertexBuffer);
        rhi.DeleteBuffer(frameData[i].indexBuffer);
        
        if (frameData[i].texelBuffer) {
            rhi.DeleteBuffer(frameData[i].texelBuffer);
        }
        
        if (frameData[i].texture) {
            textureManager.DestroyTexture(frameData[i].texture);
        }
    }
}

void BufferCacheManager::MapBufferSet(FrameDataBufferSet &bufferSet) {
#if PINNED_MEMORY
#if PERSISTENT_MAP
    RHI::BufferLockMode lockMode = RHI::WriteOnlyPersistent;
#else
    RHI::BufferLockMode lockMode = RHI::WriteOnly;
#endif

    if (!bufferSet.mappedVertexBase) {
        rhi.BindBuffer(RHI::VertexBuffer, bufferSet.vertexBuffer);
        bufferSet.mappedVertexBase = rhi.MapBufferRange(bufferSet.vertexBuffer, lockMode);
        rhi.BindBuffer(RHI::VertexBuffer, RHI::NullBuffer);
    }

    if (!bufferSet.mappedIndexBase) {
        rhi.BindBuffer(RHI::IndexBuffer, bufferSet.indexBuffer);
        bufferSet.mappedIndexBase = rhi.MapBufferRange(bufferSet.indexBuffer, lockMode);
        rhi.BindBuffer(RHI::IndexBuffer, RHI::NullBuffer);
    }

    if (!bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        rhi.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        bufferSet.mappedTexelBase = rhi.MapBufferRange(bufferSet.texelBuffer, lockMode);
        rhi.BindBuffer(bufferSet.texelBufferType, RHI::NullBuffer);
    }
#endif
}

void BufferCacheManager::UnmapBufferSet(FrameDataBufferSet &bufferSet) {
#if PINNED_MEMORY
    if (bufferSet.mappedVertexBase) {
        rhi.BindBuffer(RHI::VertexBuffer, bufferSet.vertexBuffer);
        rhi.UnmapBuffer(bufferSet.vertexBuffer);
        rhi.BindBuffer(RHI::VertexBuffer, RHI::NullBuffer);
        bufferSet.mappedVertexBase = nullptr;
    }

    if (bufferSet.mappedIndexBase) {
        rhi.BindBuffer(RHI::IndexBuffer, bufferSet.indexBuffer);
        rhi.UnmapBuffer(bufferSet.indexBuffer);
        rhi.BindBuffer(RHI::IndexBuffer, RHI::NullBuffer);
        bufferSet.mappedIndexBase = nullptr;
    }

    if (bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        rhi.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        rhi.UnmapBuffer(bufferSet.texelBuffer);
        rhi.BindBuffer(bufferSet.texelBufferType, RHI::NullBuffer);
        bufferSet.mappedTexelBase = nullptr;
    }
#endif
}

void BufferCacheManager::BeginWrite() {
#if PINNED_MEMORY
    // Wait until the gpu is no longer using the buffer
    if (frameData[mappedNum].sync != RHI::NullSync) {
        rhi.WaitSync(frameData[mappedNum].sync);
    }
#endif
}

void BufferCacheManager::EndDrawCommand() {
#if PINNED_MEMORY
    if (frameData[unmappedNum].sync != RHI::NullSync) {
        rhi.DeleteSync(frameData[unmappedNum].sync);
    }
    // Place a fence which will be removed when the draw command has finished
    frameData[unmappedNum].sync = rhi.FenceSync();
#endif
}

void BufferCacheManager::BeginBackEnd() {
    mostUsedVertexMem = Max(mostUsedVertexMem, (int)frameData[mappedNum].vertexMemUsed.GetValue());
    mostUsedIndexMem = Max(mostUsedIndexMem, (int)frameData[mappedNum].indexMemUsed.GetValue());
    mostUsedTexelMem = Max(mostUsedTexelMem, (int)frameData[mappedNum].texelMemUsed.GetValue());

    if (r_showBufferCache.GetBool()) {
        BE_LOG(L"%08d: %d alloc, vMem(%hs), iMem(%hs), tMem(%hs) : vMem(%hs), iMem(%hs), tMem(%hs)\n",
            frameCount, frameData[mappedNum].allocations,
            Str::FormatBytes(frameData[mappedNum].vertexMemUsed.GetValue()).c_str(),
            Str::FormatBytes(frameData[mappedNum].indexMemUsed.GetValue()).c_str(),
            Str::FormatBytes(frameData[mappedNum].texelMemUsed.GetValue()).c_str(),
            Str::FormatBytes(mostUsedVertexMem).c_str(),
            Str::FormatBytes(mostUsedIndexMem).c_str(),
            Str::FormatBytes(mostUsedTexelMem).c_str());
    }

#if PINNED_MEMORY && !PERSISTENT_MAP
    // unmap the current frame so the GPU can read it
    const uint32_t startUnmap = PlatformTime::Milliseconds();
    UnmapBufferSet(frameData[mappedNum]);
    const uint32_t endUnmap = PlatformTime::Milliseconds();
    if (r_showBufferCacheTiming.GetBool() && endUnmap - startUnmap > 1) {
        BE_DLOG(L"BufferCacheManager::BeginBackEnd: unmap took %i msec\n", endUnmap - startUnmap);
    }
#endif

    unmappedNum = mappedNum;

    // update buffered texture
    if (renderGlobal.skinningMethod == Mesh::VtfSkinning) {
        if (renderGlobal.vtUpdateMethod == Mesh::TboUpdate) {
            // The update to the data is not guaranteed to affect the texture until next time it is bound to a texture image unit
            rhi.SelectTextureUnit(0);
            frameData[unmappedNum].texture->Bind();
        }  else if (renderGlobal.vtUpdateMethod == Mesh::PboUpdate) {
            // unmapped PBO -> texture
            UpdatePBOTexture();
        }
    }

    // prepare the next frame for writing to by the CPU
    frameCount++;
    mappedNum = frameCount % COUNT_OF(frameData);
    
#if PINNED_MEMORY && !PERSISTENT_MAP
    const uint32_t startMap = PlatformTime::Milliseconds();
    MapBufferSet(frameData[mappedNum]);
    const uint32_t endMap = PlatformTime::Milliseconds();
    if (r_showBufferCacheTiming.GetBool() && endMap - startMap > 1) {
        BE_DLOG(L"BufferCacheManager::BeginBackEnd: map took %i msec\n", endMap - startMap);
    }
#endif

    // clear current frame data
    rhi.BufferRewind(frameData[mappedNum].vertexBuffer);
    frameData[mappedNum].vertexMemUsed.SetValue(0);

    rhi.BufferRewind(frameData[mappedNum].indexBuffer);
    frameData[mappedNum].indexMemUsed.SetValue(0);

    if (frameData[mappedNum].texelBuffer) {
        rhi.BufferRewind(frameData[mappedNum].texelBuffer);
        frameData[mappedNum].texelMemUsed.SetValue(0);
    }

    frameData[mappedNum].allocations = 0;
}

void BufferCacheManager::AllocStaticVertex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::VertexBuffer, RHI::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticIndex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::IndexBuffer, RHI::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticTexel(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::TexelBuffer, RHI::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

static void WriteBuffer(void *dst, const void *src, int numBytes) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src);
    memcpy(dst, src, numBytes);
}

bool BufferCacheManager::AllocVertex(int numVertexes, int vertexSize, const void *data, BufferCache *bc) {
    int bytes = vertexSize * numVertexes;

    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    // thread safe interlocked adds
    currentBufferSet->vertexMemUsed.Add(bytes);

    //rhi.BindBuffer(RHI::VertexBuffer, currentBufferSet->vertexBuffer);
    // Check just write offset (don't write)
    int offset = rhi.BufferWrite(currentBufferSet->vertexBuffer, vertexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR(L"Out of vertex cache");
        return false;
    }
    //rhi.BindBuffer(RHI::VertexBuffer, RHI::NullBuffer);

    currentBufferSet->allocations++;

    if (data) {
#if PINNED_MEMORY
        assert(currentBufferSet->mappedVertexBase);
        WriteBuffer((byte *)currentBufferSet->mappedVertexBase + offset, data, bytes);
#else
        rhi.BindBuffer(RHI::VertexBuffer, currentBufferSet->vertexBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->vertexBuffer, RHI::WriteOnly, offset, bytes);

        WriteBuffer((byte *)base, data, bytes);
        
        rhi.UnmapBuffer(currentBufferSet->vertexBuffer);
        rhi.BindBuffer(RHI::VertexBuffer, RHI::NullBuffer);
#endif
    }

    bc->buffer = currentBufferSet->vertexBuffer;
    bc->offset = offset;
    bc->bytes = bytes;
    bc->frameCount = frameCount;
    return true;
}

bool BufferCacheManager::AllocIndex(int numIndexes, int indexSize, const void *data, BufferCache *bc) {
    int bytes = numIndexes * indexSize;
    
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    // thread safe interlocked adds
    currentBufferSet->indexMemUsed.Add(bytes);

    //rhi.BindBuffer(RHI::IndexBuffer, currentBufferSet->indexBuffer);
    // Check just write offset (don't write)
    int offset = rhi.BufferWrite(currentBufferSet->indexBuffer, indexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR(L"Out of index cache");
        return false;
    }
    //rhi.BindBuffer(RHI::IndexBuffer, RHI::NullBuffer);

    currentBufferSet->allocations++;

    if (data) {
#if PINNED_MEMORY
        assert(currentBufferSet->mappedIndexBase);
        WriteBuffer((byte *)currentBufferSet->mappedIndexBase + offset, data, bytes);
#else
        rhi.BindBuffer(RHI::IndexBuffer, currentBufferSet->indexBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->indexBuffer, RHI::WriteOnly, offset, bytes);

        WriteBuffer((byte *)base, data, bytes);

        rhi.UnmapBuffer(currentBufferSet->indexBuffer);
        rhi.BindBuffer(RHI::IndexBuffer, RHI::NullBuffer);
#endif
    }

    bc->buffer = currentBufferSet->indexBuffer;
    bc->offset = offset;
    bc->bytes = bytes;
    bc->frameCount = frameCount;
    return true;
}

bool BufferCacheManager::AllocTexel(int bytes, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(currentBufferSet->texelBuffer);

    // thread safe interlocked adds
    currentBufferSet->texelMemUsed.Add(bytes);

    //rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    // Check just write offset (don't write)
    int offset = rhi.BufferWrite(currentBufferSet->texelBuffer, TB_BPP, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR(L"Out of texel cache");
        return false;
    }
    //rhi.BindBuffer(currentBufferSet->texelBufferType, RHI::NullBuffer);

    currentBufferSet->allocations++;

    if (data) {
#if PINNED_MEMORY
        assert(currentBufferSet->mappedTexelBase);
        WriteBuffer((byte *)currentBufferSet->mappedTexelBase + offset, data, bytes);
#else
        rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->texelBuffer, RHI::WriteOnly, offset, bytes);

        WriteBuffer((byte *)base, data, bytes);

        rhi.UnmapBuffer(currentBufferSet->texelBuffer);
        rhi.BindBuffer(currentBufferSet->texelBufferType, RHI::NullBuffer);
#endif
    }

    if (renderGlobal.vtUpdateMethod == Mesh::TboUpdate) {
        bc->tcBase[0] = offset / TB_BPP;
        bc->tcBase[1] = 0;
        bc->texture = currentBufferSet->texture;
    } else if (renderGlobal.vtUpdateMethod == Mesh::PboUpdate) {
        int texelOffset = offset / TB_BPP;
        bc->tcBase[0] = texelOffset % TB_WIDTH;
        bc->tcBase[1] = texelOffset / TB_WIDTH;
        bc->texture = frameData[0].texture;

        pboWriteOffset = offset + bytes;
    }

    bc->buffer = currentBufferSet->texelBuffer;
    bc->bytes = bytes;
    bc->frameCount = frameCount;

    return true;
}

byte *BufferCacheManager::MapVertexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);

#if PINNED_MEMORY
    return (byte *)currentBufferSet->mappedVertexBase + bc->offset;
#else
    rhi.BindBuffer(RHI::VertexBuffer, currentBufferSet->vertexBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->vertexBuffer, RHI::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if PINNED_MEMORY
    return (byte *)currentBufferSet->mappedIndexBase + bc->offset;
#else
    rhi.BindBuffer(RHI::IndexBuffer, currentBufferSet->indexBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->indexBuffer, RHI::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if PINNED_MEMORY
    return (byte *)currentBufferSet->mappedTexelBase + bc->offset;
#else
    rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->texelBuffer, RHI::WriteOnly, bc->offset, bc->bytes);
#endif
}

void BufferCacheManager::UnmapVertexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if PINNED_MEMORY
    //rhi.BindBuffer(RHI::VertexBuffer, currentBufferSet->vertexBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->vertexBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->vertexBuffer);
#endif
}

void BufferCacheManager::UnmapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if PINNED_MEMORY
    //rhi.BindBuffer(RHI::IndexBuffer, currentBufferSet->indexBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->indexBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->indexBuffer);
#endif
}

void BufferCacheManager::UnmapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if PINNED_MEMORY
    //rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->texelBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->texelBuffer);
#endif
}

bool BufferCacheManager::IsCached(const BufferCache *bc) const {
    if (bc->frameCount == 0xFFFFFFFF) { // static buffer
        return true;
    }

    if (bc->buffer != RHI::NullBuffer && bc->frameCount == frameCount) {
        return true;
    }

    return false;
}

void BufferCacheManager::UpdatePBOTexture() const {
    const FrameDataBufferSet *currentBufferSet = &frameData[unmappedNum];

    int texelOffset = pboWriteOffset / TB_BPP;

    int updateW = Min(texelOffset, TB_WIDTH);
    int updateH = (texelOffset + TB_WIDTH - 1) / TB_WIDTH;

    if (updateW * updateH > 0) {
        frameData[0].texture->Bind();

        // Transfer PBO -> texture using DMA
        // If asynchronous DMA transfer is supported, glTexSubImage2D() should return immediately.
        rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);

        const uint32_t startUpdatePBO = PlatformTime::Milliseconds();

        frameData[0].texture->Update2D(0, 0, updateW, updateH, Image::RGBA_32F_32F_32F_32F, nullptr);

        const uint32_t endUpdatePBO = PlatformTime::Milliseconds();
        if (endUpdatePBO - startUpdatePBO > 1) {
            BE_DLOG(L"BufferCacheManager::UpdatePBOTexture: update pbo took %i msec\n", endUpdatePBO - startUpdatePBO);
        }

        rhi.BindBuffer(currentBufferSet->texelBufferType, RHI::NullBuffer);
    }
}

const Texture *BufferCacheManager::GetFrameTexture() const {
    if (renderGlobal.vtUpdateMethod == Mesh::PboUpdate) {
        return frameData[0].texture;
    }
    return frameData[unmappedNum].texture;
}

BE_NAMESPACE_END
