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
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

#define USE_PINNED_MEMORY   1

BufferCacheManager      bufferCacheManager;

static constexpr int    TB_WIDTH    = 4096;
static constexpr int    TB_HEIGHT   = 32;
static constexpr int    TB_BPP      = 4 * sizeof(float); // size of float RGBA
static constexpr int    TB_PITCH    = TB_BPP * TB_WIDTH;
static constexpr int    TB_BYTES    = TB_PITCH * TB_HEIGHT;

void BufferCacheManager::Init() {
    usePersistentMappedBuffers = rhi.SupportsBufferStorage();
    useFlushMappedBuffers = true;

    int vcSize = r_dynamicVertexCacheSize.GetInteger();
    int icSize = r_dynamicIndexCacheSize.GetInteger();
    int ucSize = r_dynamicUniformCacheSize.GetInteger();

    // Create dynamic buffer to use for dynamic batching.
    for (int frameDataIndex = 0; frameDataIndex < COUNT_OF(frameData); frameDataIndex++) {
        FrameDataBufferSet *bufferSet = &frameData[frameDataIndex];
        
        memset(bufferSet, 0, sizeof(frameData[0]));

        bufferSet->vertexBuffer = rhi.CreateBuffer(RHI::BufferType::Vertex, RHI::BufferUsage::Dynamic, vcSize, 0, nullptr);
        bufferSet->indexBuffer = rhi.CreateBuffer(RHI::BufferType::Index, RHI::BufferUsage::Dynamic, icSize, 0, nullptr);
        bufferSet->uniformBuffer = rhi.CreateBuffer(RHI::BufferType::Uniform, RHI::BufferUsage::Dynamic, ucSize, 0, nullptr);

        if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
            // Create texture buffer to write directly.
            bufferSet->texelBufferType = RHI::BufferType::Texel;
            bufferSet->texelBuffer = rhi.CreateBuffer(bufferSet->texelBufferType, RHI::BufferUsage::Dynamic, TB_BYTES, 0, nullptr);
            bufferSet->texture = textureManager.AllocTexture(va("_tbTexture%i", frameDataIndex));
            bufferSet->texture->CreateFromBuffer(Image::Format::RGBA_32F_32F_32F_32F, bufferSet->texelBuffer);
        } else if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
            // Create unpack buffer to translate data from PBO to VTF texture.
            // See below link if you want to know what PBO is.
            // http://www.songho.ca/opengl/gl_pbo.html
            bufferSet->texelBufferType = RHI::BufferType::PixelUnpack;
            bufferSet->texelBuffer = rhi.CreateBuffer(bufferSet->texelBufferType, RHI::BufferUsage::Dynamic, TB_BYTES, TB_PITCH, nullptr);
            if (frameDataIndex == 0) {
                bufferSet->texture = textureManager.AllocTexture("_tbTexture");
                bufferSet->texture->CreateEmpty(RHI::TextureType::Texture2D, TB_WIDTH, TB_HEIGHT, 1, 1, 1,
                    Image::Format::RGBA_32F_32F_32F_32F, Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::HighPriority);
            }
        }

#if USE_PINNED_MEMORY
        frameData[frameDataIndex].sync = rhi.CreateSync();
#endif
    }
    
    BE_LOG("dynamic vertex buffer created (%s x %i)\n", Str::FormatBytes(vcSize).c_str(), COUNT_OF(frameData));
    BE_LOG("dynamic index buffer created (%s x %i)\n", Str::FormatBytes(icSize).c_str(), COUNT_OF(frameData));
    BE_LOG("dynamic uniform buffer created (%s x %i)\n", Str::FormatBytes(ucSize).c_str(), COUNT_OF(frameData));

    if (frameData[0].texelBuffer) {
        BE_LOG("dynamic texel buffer created (%s x %i)\n", Str::FormatBytes(TB_BYTES).c_str(), COUNT_OF(frameData));
    }
    
    // Create stream buffer for use in debug drawing.
    streamVertexBuffer = rhi.CreateBuffer(RHI::BufferType::Vertex, RHI::BufferUsage::Stream, 0);
    streamIndexBuffer = rhi.CreateBuffer(RHI::BufferType::Index, RHI::BufferUsage::Stream, 0);
    streamUniformBuffer = rhi.CreateBuffer(RHI::BufferType::Uniform, RHI::BufferUsage::Stream, 0);

    pboWriteOffset = 0;

    frameCount = 0;
    mappedNum = 0;
    unmappedNum = -1;

    mostUsedVertexMem = 0;
    mostUsedIndexMem = 0;
    mostUsedUniformMem = 0;
    mostUsedTexelMem = 0;

#if USE_PINNED_MEMORY
    MapBufferSet(frameData[mappedNum]);
        
    if (usePersistentMappedBuffers) {
        for (int i = mappedNum + 1; i < COUNT_OF(frameData); i++) {
            MapBufferSet(frameData[i]);
        }
    }
#endif
}

void BufferCacheManager::Shutdown() {
    rhi.DestroyBuffer(streamVertexBuffer);
    rhi.DestroyBuffer(streamIndexBuffer);
    rhi.DestroyBuffer(streamUniformBuffer);

    for (int i = 0; i < COUNT_OF(frameData); i++) {
#if USE_PINNED_MEMORY
        UnmapBufferSet(frameData[i], false);

        if (frameData[i].sync) {
            rhi.DestroySync(frameData[i].sync);
        }
#endif

        rhi.DestroyBuffer(frameData[i].vertexBuffer);
        rhi.DestroyBuffer(frameData[i].indexBuffer);
        rhi.DestroyBuffer(frameData[i].uniformBuffer);
        
        if (frameData[i].texelBuffer) {
            rhi.DestroyBuffer(frameData[i].texelBuffer);
        }
        
        if (frameData[i].texture) {
            textureManager.DestroyTexture(frameData[i].texture);
        }
    }
}

void BufferCacheManager::MapBufferSet(FrameDataBufferSet &bufferSet) {
    BE_SCOPE_PROFILE_CPU("BufferCacheManager::MapBufferSet");

    RHI::BufferLockMode::Enum lockMode = usePersistentMappedBuffers ? RHI::BufferLockMode::WriteOnlyPersistent : (useFlushMappedBuffers ? RHI::BufferLockMode::WriteOnlyExplicitFlush : RHI::BufferLockMode::WriteOnly);
    
    if (!bufferSet.mappedVertexBase) {
        rhi.BindBuffer(RHI::BufferType::Vertex, bufferSet.vertexBuffer);
        bufferSet.mappedVertexBase = rhi.MapBuffer(bufferSet.vertexBuffer, lockMode);
        rhi.BindBuffer(RHI::BufferType::Vertex, RHI::NullBuffer);
    }

    if (!bufferSet.mappedIndexBase) {
        rhi.BindBuffer(RHI::BufferType::Index, bufferSet.indexBuffer);
        bufferSet.mappedIndexBase = rhi.MapBuffer(bufferSet.indexBuffer, lockMode);
        rhi.BindBuffer(RHI::BufferType::Index, RHI::NullBuffer);
    }

    if (!bufferSet.mappedUniformBase) {
        rhi.BindBuffer(RHI::BufferType::Uniform, bufferSet.uniformBuffer);
        bufferSet.mappedUniformBase = rhi.MapBuffer(bufferSet.uniformBuffer, lockMode);
        rhi.BindBuffer(RHI::BufferType::Uniform, RHI::NullBuffer);
    }

    if (!bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        rhi.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        bufferSet.mappedTexelBase = rhi.MapBuffer(bufferSet.texelBuffer, lockMode);
        rhi.BindBuffer(bufferSet.texelBufferType, RHI::NullBuffer);
    }
}

void BufferCacheManager::UnmapBufferSet(FrameDataBufferSet &bufferSet, bool flush) {
    BE_SCOPE_PROFILE_CPU("BufferCacheManager::UnmapBufferSet");

    if (bufferSet.mappedVertexBase) {
        rhi.BindBuffer(RHI::BufferType::Vertex, bufferSet.vertexBuffer);
        if (flush && bufferSet.vertexMemUsed.load() > 0) {
            rhi.FlushMappedBufferRange(bufferSet.vertexBuffer, 0, bufferSet.vertexMemUsed.load());
        }
        rhi.UnmapBuffer(bufferSet.vertexBuffer);
        rhi.BindBuffer(RHI::BufferType::Vertex, RHI::NullBuffer);
        bufferSet.mappedVertexBase = nullptr;
    }

    if (bufferSet.mappedIndexBase) {
        rhi.BindBuffer(RHI::BufferType::Index, bufferSet.indexBuffer);
        if (flush && bufferSet.indexMemUsed.load() > 0) {
            rhi.FlushMappedBufferRange(bufferSet.indexBuffer, 0, bufferSet.indexMemUsed.load());
        }
        rhi.UnmapBuffer(bufferSet.indexBuffer);
        rhi.BindBuffer(RHI::BufferType::Index, RHI::NullBuffer);
        bufferSet.mappedIndexBase = nullptr;
    }

    if (bufferSet.mappedUniformBase) {
        rhi.BindBuffer(RHI::BufferType::Uniform, bufferSet.uniformBuffer);
        if (flush && bufferSet.uniformMemUsed.load() > 0) {
            rhi.FlushMappedBufferRange(bufferSet.uniformBuffer, 0, bufferSet.uniformMemUsed.load());
        }
        rhi.UnmapBuffer(bufferSet.uniformBuffer);
        rhi.BindBuffer(RHI::BufferType::Uniform, RHI::NullBuffer);
        bufferSet.mappedUniformBase = nullptr;
    }

    if (bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        rhi.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        if (flush && bufferSet.texelMemUsed.load() > 0) {
            rhi.FlushMappedBufferRange(bufferSet.texelBuffer, 0, bufferSet.texelMemUsed.load());
        }
        rhi.UnmapBuffer(bufferSet.texelBuffer);
        rhi.BindBuffer(bufferSet.texelBufferType, RHI::NullBuffer);
        bufferSet.mappedTexelBase = nullptr;
    }
}

void BufferCacheManager::BeginWrite() {
    BE_SCOPE_PROFILE_CPU("BufferCacheManager::BeginWrite");

#if USE_PINNED_MEMORY
    // Wait until the gpu is no longer using the buffer.
    if (rhi.IsSync(frameData[mappedNum].sync)) {
        rhi.WaitSync(frameData[mappedNum].sync);
    }
#endif
}

void BufferCacheManager::EndWrite() {
    BE_SCOPE_PROFILE_CPU("BufferCacheManager::EndWrite");

#if USE_PINNED_MEMORY
    if (rhi.IsSync(frameData[unmappedNum].sync)) {
        rhi.DeleteSync(frameData[unmappedNum].sync);
    }
    // Place a fence which will be removed when the draw command has finished.
    rhi.FenceSync(frameData[unmappedNum].sync);
#endif
}

void BufferCacheManager::BeginBackEnd() {
    BE_SCOPE_PROFILE_CPU("BufferCacheManager::BeginBackEnd");

    mostUsedVertexMem = Max(mostUsedVertexMem, (int)frameData[mappedNum].vertexMemUsed.load());
    mostUsedIndexMem = Max(mostUsedIndexMem, (int)frameData[mappedNum].indexMemUsed.load());
    mostUsedUniformMem = Max(mostUsedUniformMem, (int)frameData[mappedNum].uniformMemUsed.load());
    mostUsedTexelMem = Max(mostUsedTexelMem, (int)frameData[mappedNum].texelMemUsed.load());

    if (r_showBufferCache.GetBool()) {
        BE_LOG("%08d: %d alloc, vMem(%s), iMem(%s), uMem(%s), tMem(%s) : vMem(%s), iMem(%s), uMem(%s), tMem(%s)\n",
            frameCount, frameData[mappedNum].allocations,
            Str::FormatBytes(frameData[mappedNum].vertexMemUsed.load()).c_str(),
            Str::FormatBytes(frameData[mappedNum].indexMemUsed.load()).c_str(),
            Str::FormatBytes(frameData[mappedNum].uniformMemUsed.load()).c_str(),
            Str::FormatBytes(frameData[mappedNum].texelMemUsed.load()).c_str(),
            Str::FormatBytes(mostUsedVertexMem).c_str(),
            Str::FormatBytes(mostUsedIndexMem).c_str(),
            Str::FormatBytes(mostUsedUniformMem).c_str(),
            Str::FormatBytes(mostUsedTexelMem).c_str());
    }

#if USE_PINNED_MEMORY
    if (!usePersistentMappedBuffers) {
        // Unmap the current frame so the GPU can read it.
        const double startUnmap = PlatformTime::Seconds();
        UnmapBufferSet(frameData[mappedNum], useFlushMappedBuffers);
        const double endUnmap = PlatformTime::Seconds();
        if (r_showBufferCacheTiming.GetBool() && endUnmap - startUnmap > 1.0) {
            BE_DLOG("BufferCacheManager::BeginBackEnd: unmap took %.3f seconds\n", endUnmap - startUnmap);
        }
    }
#endif

    unmappedNum = mappedNum;

    // Update buffered texture.
    if (renderGlobal.skinningMethod == SkinningJointCache::SkinningMethod::VertexTextureFetch) {
        if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
            // The update to the data is not guaranteed to affect the texture until next time it is bound to a texture image unit.
            rhi.SelectTextureUnit(0);
            frameData[unmappedNum].texture->Bind();
        }  else if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
            // Unmapped PBO -> texture
            UpdatePBOTexture();
        }
    }

    // Prepare the next frame for writing to by the CPU.
    frameCount++;
    mappedNum = frameCount % COUNT_OF(frameData);

#if USE_PINNED_MEMORY
    if (!usePersistentMappedBuffers) {
        const double startMap = PlatformTime::Seconds();
        MapBufferSet(frameData[mappedNum]);
        const double endMap = PlatformTime::Seconds();
        if (r_showBufferCacheTiming.GetBool() && endMap - startMap > 1.0) {
            BE_DLOG("BufferCacheManager::BeginBackEnd: map took %.3f seconds\n", endMap - startMap);
        }
    }
#endif

    // Clear current frame data.
    rhi.BufferRewind(frameData[mappedNum].vertexBuffer);
    frameData[mappedNum].vertexMemUsed = 0;

    rhi.BufferRewind(frameData[mappedNum].indexBuffer);
    frameData[mappedNum].indexMemUsed = 0;

    rhi.BufferRewind(frameData[mappedNum].uniformBuffer);
    frameData[mappedNum].uniformMemUsed = 0;

    if (frameData[mappedNum].texelBuffer) {
        rhi.BufferRewind(frameData[mappedNum].texelBuffer);
        frameData[mappedNum].texelMemUsed = 0;
    }

    frameData[mappedNum].allocations = 0;
}

void BufferCacheManager::AllocStaticVertex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::BufferType::Vertex, RHI::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticIndex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::BufferType::Index, RHI::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticUniform(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::BufferType::Uniform, RHI::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticTexel(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = rhi.CreateBuffer(RHI::BufferType::Texel, RHI::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

bool BufferCacheManager::AllocVertex(int numVertexes, int vertexSize, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    int bytes = vertexSize * numVertexes;

    // Check just write offset (don't write).
    int offset = rhi.BufferWrite(currentBufferSet->vertexBuffer, vertexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of vertex cache");
        return false;
    }

    currentBufferSet->vertexMemUsed = offset + bytes;
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedVertexBase);
        rhi.WriteBuffer((byte *)currentBufferSet->mappedVertexBase + offset, (byte *)data, bytes);
#else
        rhi.BindBuffer(RHI::BufferType::Vertex, currentBufferSet->vertexBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->vertexBuffer, RHI::BufferLockMode::WriteOnly, offset, bytes);

        rhi.WriteBuffer((byte *)base, (byte *)data, bytes);
        
        rhi.UnmapBuffer(currentBufferSet->vertexBuffer);
        rhi.BindBuffer(RHI::BufferType::Vertex, RHI::NullBuffer);
#endif
    }

    bc->buffer = currentBufferSet->vertexBuffer;
    bc->offset = offset;
    bc->bytes = bytes;
    bc->frameCount = frameCount;
    return true;
}

bool BufferCacheManager::AllocIndex(int numIndexes, int indexSize, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    int bytes = numIndexes * indexSize;

    // Check just write offset (don't write).
    int offset = rhi.BufferWrite(currentBufferSet->indexBuffer, indexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of index cache");
        return false;
    }

    currentBufferSet->indexMemUsed = offset + bytes;
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedIndexBase);
        rhi.WriteBuffer((byte *)currentBufferSet->mappedIndexBase + offset, (byte *)data, bytes);
#else
        rhi.BindBuffer(RHI::BufferType::Index, currentBufferSet->indexBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->indexBuffer, RHI::BufferLockMode::WriteOnly, offset, bytes);

        rhi.WriteBuffer((byte *)base, (byte *)data, bytes);

        rhi.UnmapBuffer(currentBufferSet->indexBuffer);
        rhi.BindBuffer(RHI::BufferType::Index, RHI::NullBuffer);
#endif
    }

    bc->buffer = currentBufferSet->indexBuffer;
    bc->offset = offset;
    bc->bytes = bytes;
    bc->frameCount = frameCount;
    return true;
}

bool BufferCacheManager::AllocUniform(int bytes, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    // Check just write offset (don't write).
    int offset = rhi.BufferWrite(currentBufferSet->uniformBuffer, rhi.HWLimit().uniformBufferOffsetAlignment, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of uniform cache");
        return false;
    }

    currentBufferSet->uniformMemUsed = offset + bytes;
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedUniformBase);
        rhi.WriteBuffer((byte *)currentBufferSet->mappedUniformBase + offset, (byte *)data, bytes);
#else
        rhi.BindBuffer(RHI::BufferType::Uniform, currentBufferSet->uniformBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->uniformBuffer, RHI::BufferLockMode::WriteOnly, offset, bytes);

        rhi.WriteBuffer((byte *)base, (byte *)data, bytes);

        rhi.UnmapBuffer(currentBufferSet->uniformBuffer);
        rhi.BindBuffer(RHI::BufferType::Uniform, RHI::NullBuffer);
#endif
    }

    bc->buffer = currentBufferSet->uniformBuffer;
    bc->offset = offset;
    bc->bytes = bytes;
    bc->frameCount = frameCount;
    return true;
}

bool BufferCacheManager::AllocTexel(int bytes, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(currentBufferSet->texelBuffer);

    // Check just write offset (don't write).
    int offset = rhi.BufferWrite(currentBufferSet->texelBuffer, TB_BPP, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of texel cache");
        return false;
    }

    currentBufferSet->texelMemUsed = offset + bytes;
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedTexelBase);
        rhi.WriteBuffer((byte *)currentBufferSet->mappedTexelBase + offset, (byte *)data, bytes);
#else
        rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
        void *base = rhi.MapBufferRange(currentBufferSet->texelBuffer, RHI::BufferLockMode::WriteOnly, offset, bytes);

        rhi.WriteBuffer((byte *)base, (byte *)data, bytes);

        rhi.UnmapBuffer(currentBufferSet->texelBuffer);
        rhi.BindBuffer(currentBufferSet->texelBufferType, RHI::NullBuffer);
#endif
    }

    if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
        bc->tcBase[0] = offset / TB_BPP;
        bc->tcBase[1] = 0;
        bc->texture = currentBufferSet->texture;
    } else if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
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

#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedVertexBase + bc->offset;
#else
    rhi.BindBuffer(RHI::BufferType::Vertex, currentBufferSet->vertexBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->vertexBuffer, RHI::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedIndexBase + bc->offset;
#else
    rhi.BindBuffer(RHI::BufferType::Index, currentBufferSet->indexBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->indexBuffer, RHI::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapUniformBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedUniformBase + bc->offset;
#else
    rhi.BindBuffer(RHI::BufferType::Uniform, currentBufferSet->uniformBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->uniformBuffer, RHI::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedTexelBase + bc->offset;
#else
    rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    return (byte *)rhi.MapBufferRange(currentBufferSet->texelBuffer, RHI::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

void BufferCacheManager::UnmapVertexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //rhi.BindBuffer(RHI::BufferType::Vertex, currentBufferSet->vertexBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->vertexBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->vertexBuffer);
#endif
}

void BufferCacheManager::UnmapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //rhi.BindBuffer(RHI::BufferType::Index, currentBufferSet->indexBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->indexBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->indexBuffer);
#endif
}

void BufferCacheManager::UnmapUniformBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //rhi.BindBuffer(RHI::BufferType::Uniform, currentBufferSet->uniformBuffer);
    //rhi.FlushMappedBufferRange(currentBufferSet->uniformBuffer, bc->offset, bc->bytes);
#else
    rhi.UnmapBuffer(currentBufferSet->uniformBuffer);
#endif
}

void BufferCacheManager::UnmapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
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

bool BufferCacheManager::IsCacheStatic(const BufferCache *bc) const {
    if (bc->frameCount == 0xFFFFFFFF) {
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

        // Transfer PBO -> texture using DMA.
        // If asynchronous DMA transfer is supported, glTexSubImage2D() should return immediately.
        rhi.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);

        const double startUpdatePBO = PlatformTime::Seconds();

        frameData[0].texture->Update2D(0, 0, 0, updateW, updateH, Image::Format::RGBA_32F_32F_32F_32F, nullptr);

        const double endUpdatePBO = PlatformTime::Seconds();
        if (endUpdatePBO - startUpdatePBO > 1) {
            BE_DLOG("BufferCacheManager::UpdatePBOTexture: update pbo took %.3f msec\n", endUpdatePBO - startUpdatePBO);
        }

        rhi.BindBuffer(currentBufferSet->texelBufferType, RHI::NullBuffer);
    }
}

const Texture *BufferCacheManager::GetFrameTexture() const {
    if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
        return frameData[0].texture;
    }
    return frameData[unmappedNum].texture;
}

BE_NAMESPACE_END
