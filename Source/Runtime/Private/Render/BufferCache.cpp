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
    usePersistentMappedBuffers = graphics.SupportsBufferStorage();
    useFlushMappedBuffers = true;

    int vcSize = r_dynamicVertexCacheSize.GetInteger();
    int icSize = r_dynamicIndexCacheSize.GetInteger();
    int ucSize = r_dynamicUniformCacheSize.GetInteger();

    // Create dynamic buffer to use for dynamic batching.
    for (int frameDataIndex = 0; frameDataIndex < COUNT_OF(frameData); frameDataIndex++) {
        FrameDataBufferSet *bufferSet = &frameData[frameDataIndex];
        
        memset(bufferSet, 0, sizeof(frameData[0]));

        bufferSet->vertexBuffer = graphics.CreateBuffer(Graphics::BufferType::Vertex, Graphics::BufferUsage::Dynamic, vcSize, 0, nullptr);
        bufferSet->indexBuffer = graphics.CreateBuffer(Graphics::BufferType::Index, Graphics::BufferUsage::Dynamic, icSize, 0, nullptr);
        bufferSet->uniformBuffer = graphics.CreateBuffer(Graphics::BufferType::Uniform, Graphics::BufferUsage::Dynamic, ucSize, 0, nullptr);

        if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
            // Create texture buffer to write directly.
            bufferSet->texelBufferType = Graphics::BufferType::Texel;
            bufferSet->texelBuffer = graphics.CreateBuffer(bufferSet->texelBufferType, Graphics::BufferUsage::Dynamic, TB_BYTES, 0, nullptr);
            bufferSet->texture = textureManager.AllocTexture(va("_tbTexture%i", frameDataIndex));
            bufferSet->texture->CreateFromBuffer(Image::Format::R32G32B32A32_FLOAT, bufferSet->texelBuffer);
        } else if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
            // Create unpack buffer to translate data from PBO to VTF texture.
            // See below link if you want to know what PBO is.
            // http://www.songho.ca/opengl/gl_pbo.html
            bufferSet->texelBufferType = Graphics::BufferType::PixelUnpack;
            bufferSet->texelBuffer = graphics.CreateBuffer(bufferSet->texelBufferType, Graphics::BufferUsage::Dynamic, TB_BYTES, TB_PITCH, nullptr);
            if (frameDataIndex == 0) {
                bufferSet->texture = textureManager.AllocTexture("_tbTexture");
                bufferSet->texture->CreateEmpty(Graphics::TextureType::Texture2D, TB_WIDTH, TB_HEIGHT, 1, 1, 1,
                    Image::Format::R32G32B32A32_FLOAT, Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::HighPriority);
            }
        }

#if USE_PINNED_MEMORY
        frameData[frameDataIndex].sync = graphics.CreateSync();
#endif
    }
    
    BE_LOG("dynamic vertex buffer created (%s x %i)\n", Str::FormatBytes(vcSize).c_str(), COUNT_OF(frameData));
    BE_LOG("dynamic index buffer created (%s x %i)\n", Str::FormatBytes(icSize).c_str(), COUNT_OF(frameData));
    BE_LOG("dynamic uniform buffer created (%s x %i)\n", Str::FormatBytes(ucSize).c_str(), COUNT_OF(frameData));

    if (frameData[0].texelBuffer) {
        BE_LOG("dynamic texel buffer created (%s x %i)\n", Str::FormatBytes(TB_BYTES).c_str(), COUNT_OF(frameData));
    }
    
    // Create stream buffer for use in debug drawing.
    streamVertexBuffer = graphics.CreateBuffer(Graphics::BufferType::Vertex, Graphics::BufferUsage::Stream, 0);
    streamIndexBuffer = graphics.CreateBuffer(Graphics::BufferType::Index, Graphics::BufferUsage::Stream, 0);
    streamUniformBuffer = graphics.CreateBuffer(Graphics::BufferType::Uniform, Graphics::BufferUsage::Stream, 0);

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
    graphics.DestroyBuffer(streamVertexBuffer);
    graphics.DestroyBuffer(streamIndexBuffer);
    graphics.DestroyBuffer(streamUniformBuffer);

    for (int i = 0; i < COUNT_OF(frameData); i++) {
#if USE_PINNED_MEMORY
        UnmapBufferSet(frameData[i], false);

        if (frameData[i].sync) {
            graphics.DestroySync(frameData[i].sync);
        }
#endif

        graphics.DestroyBuffer(frameData[i].vertexBuffer);
        graphics.DestroyBuffer(frameData[i].indexBuffer);
        graphics.DestroyBuffer(frameData[i].uniformBuffer);
        
        if (frameData[i].texelBuffer) {
            graphics.DestroyBuffer(frameData[i].texelBuffer);
        }
        
        if (frameData[i].texture) {
            textureManager.DestroyTexture(frameData[i].texture);
        }
    }
}

void BufferCacheManager::MapBufferSet(FrameDataBufferSet &bufferSet) {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::MapBufferSet");

    Graphics::BufferLockMode::Enum lockMode = usePersistentMappedBuffers ? Graphics::BufferLockMode::WriteOnlyPersistent : (useFlushMappedBuffers ? Graphics::BufferLockMode::WriteOnlyExplicitFlush : Graphics::BufferLockMode::WriteOnly);
    
    if (!bufferSet.mappedVertexBase) {
        graphics.BindBuffer(Graphics::BufferType::Vertex, bufferSet.vertexBuffer);
        bufferSet.mappedVertexBase = graphics.MapBuffer(bufferSet.vertexBuffer, lockMode);
        graphics.BindBuffer(Graphics::BufferType::Vertex, Graphics::NullBuffer);
    }

    if (!bufferSet.mappedIndexBase) {
        graphics.BindBuffer(Graphics::BufferType::Index, bufferSet.indexBuffer);
        bufferSet.mappedIndexBase = graphics.MapBuffer(bufferSet.indexBuffer, lockMode);
        graphics.BindBuffer(Graphics::BufferType::Index, Graphics::NullBuffer);
    }

    if (!bufferSet.mappedUniformBase) {
        graphics.BindBuffer(Graphics::BufferType::Uniform, bufferSet.uniformBuffer);
        bufferSet.mappedUniformBase = graphics.MapBuffer(bufferSet.uniformBuffer, lockMode);
        graphics.BindBuffer(Graphics::BufferType::Uniform, Graphics::NullBuffer);
    }

    if (!bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        graphics.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        bufferSet.mappedTexelBase = graphics.MapBuffer(bufferSet.texelBuffer, lockMode);
        graphics.BindBuffer(bufferSet.texelBufferType, Graphics::NullBuffer);
    }
}

void BufferCacheManager::UnmapBufferSet(FrameDataBufferSet &bufferSet, bool flush) {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::UnmapBufferSet");

    if (bufferSet.mappedVertexBase) {
        graphics.BindBuffer(Graphics::BufferType::Vertex, bufferSet.vertexBuffer);
        if (flush && bufferSet.vertexMemUsed.load() > 0) {
            graphics.FlushMappedBufferRange(bufferSet.vertexBuffer, 0, bufferSet.vertexMemUsed.load());
        }
        graphics.UnmapBuffer(bufferSet.vertexBuffer);
        graphics.BindBuffer(Graphics::BufferType::Vertex, Graphics::NullBuffer);
        bufferSet.mappedVertexBase = nullptr;
    }

    if (bufferSet.mappedIndexBase) {
        graphics.BindBuffer(Graphics::BufferType::Index, bufferSet.indexBuffer);
        if (flush && bufferSet.indexMemUsed.load() > 0) {
            graphics.FlushMappedBufferRange(bufferSet.indexBuffer, 0, bufferSet.indexMemUsed.load());
        }
        graphics.UnmapBuffer(bufferSet.indexBuffer);
        graphics.BindBuffer(Graphics::BufferType::Index, Graphics::NullBuffer);
        bufferSet.mappedIndexBase = nullptr;
    }

    if (bufferSet.mappedUniformBase) {
        graphics.BindBuffer(Graphics::BufferType::Uniform, bufferSet.uniformBuffer);
        if (flush && bufferSet.uniformMemUsed.load() > 0) {
            graphics.FlushMappedBufferRange(bufferSet.uniformBuffer, 0, bufferSet.uniformMemUsed.load());
        }
        graphics.UnmapBuffer(bufferSet.uniformBuffer);
        graphics.BindBuffer(Graphics::BufferType::Uniform, Graphics::NullBuffer);
        bufferSet.mappedUniformBase = nullptr;
    }

    if (bufferSet.mappedTexelBase && bufferSet.texelBuffer) {
        graphics.BindBuffer(bufferSet.texelBufferType, bufferSet.texelBuffer);
        if (flush && bufferSet.texelMemUsed.load() > 0) {
            graphics.FlushMappedBufferRange(bufferSet.texelBuffer, 0, bufferSet.texelMemUsed.load());
        }
        graphics.UnmapBuffer(bufferSet.texelBuffer);
        graphics.BindBuffer(bufferSet.texelBufferType, Graphics::NullBuffer);
        bufferSet.mappedTexelBase = nullptr;
    }
}

void BufferCacheManager::BeginWrite() {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::BeginWrite");

#if USE_PINNED_MEMORY
    // Wait until the gpu is no longer using the buffer.
    if (graphics.IsSync(frameData[mappedNum].sync)) {
        graphics.WaitSync(frameData[mappedNum].sync);
    }
#endif
}

void BufferCacheManager::EndWrite() {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::EndWrite");

#if USE_PINNED_MEMORY
    if (graphics.IsSync(frameData[unmappedNum].sync)) {
        graphics.DeleteSync(frameData[unmappedNum].sync);
    }
    // Place a fence which will be removed when the draw command has finished.
    graphics.FenceSync(frameData[unmappedNum].sync);
#endif
}

void BufferCacheManager::BeginBackEnd() {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::BeginBackEnd");

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
            graphics.SelectTextureUnit(0);
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
    graphics.BufferRewind(frameData[mappedNum].vertexBuffer);
    frameData[mappedNum].vertexMemUsed.store(0);

    graphics.BufferRewind(frameData[mappedNum].indexBuffer);
    frameData[mappedNum].indexMemUsed.store(0);

    graphics.BufferRewind(frameData[mappedNum].uniformBuffer);
    frameData[mappedNum].uniformMemUsed.store(0);

    if (frameData[mappedNum].texelBuffer) {
        graphics.BufferRewind(frameData[mappedNum].texelBuffer);
        frameData[mappedNum].texelMemUsed.store(0);
    }

    frameData[mappedNum].allocations = 0;
}

void BufferCacheManager::AllocStaticVertex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = graphics.CreateBuffer(Graphics::BufferType::Vertex, Graphics::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticIndex(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = graphics.CreateBuffer(Graphics::BufferType::Index, Graphics::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticUniform(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = graphics.CreateBuffer(Graphics::BufferType::Uniform, Graphics::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

void BufferCacheManager::AllocStaticTexel(int bytes, const void *data, BufferCache *bc) {
    bc->buffer = graphics.CreateBuffer(Graphics::BufferType::Texel, Graphics::BufferUsage::Static, bytes, 0, data);
    bc->offset = 0;
    bc->bytes = bytes;
    bc->frameCount = 0xFFFFFFFF;
}

bool BufferCacheManager::AllocVertex(int numVertexes, int vertexSize, const void *data, BufferCache *bc) {
    FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];

    int bytes = vertexSize * numVertexes;

    // Check just write offset (don't write).
    int offset = graphics.BufferWrite(currentBufferSet->vertexBuffer, vertexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of vertex cache");
        return false;
    }

    currentBufferSet->vertexMemUsed.store(offset + bytes);
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedVertexBase);
        simdProcessor->MemcpyStream((byte *)currentBufferSet->mappedVertexBase + offset, (byte *)data, bytes);
#else
        graphics.BindBuffer(Graphics::BufferType::Vertex, currentBufferSet->vertexBuffer);
        void *base = graphics.MapBufferRange(currentBufferSet->vertexBuffer, Graphics::BufferLockMode::WriteOnly, offset, bytes);

        simdProcessor->MemcpyStream((byte *)base, (byte *)data, bytes);
        
        graphics.UnmapBuffer(currentBufferSet->vertexBuffer);
        graphics.BindBuffer(Graphics::BufferType::Vertex, Graphics::NullBuffer);
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
    int offset = graphics.BufferWrite(currentBufferSet->indexBuffer, indexSize, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of index cache");
        return false;
    }

    currentBufferSet->indexMemUsed.store(offset + bytes);
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedIndexBase);
        simdProcessor->MemcpyStream((byte *)currentBufferSet->mappedIndexBase + offset, (byte *)data, bytes);
#else
        graphics.BindBuffer(Graphics::BufferType::Index, currentBufferSet->indexBuffer);
        void *base = graphics.MapBufferRange(currentBufferSet->indexBuffer, Graphics::BufferLockMode::WriteOnly, offset, bytes);

        simdProcessor->MemcpyStream((byte *)base, (byte *)data, bytes);

        graphics.UnmapBuffer(currentBufferSet->indexBuffer);
        graphics.BindBuffer(Graphics::BufferType::Index, Graphics::NullBuffer);
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
    int offset = graphics.BufferWrite(currentBufferSet->uniformBuffer, graphics.HWLimit().uniformBufferOffsetAlignment, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of uniform cache");
        return false;
    }

    currentBufferSet->uniformMemUsed.store(offset + bytes);
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedUniformBase);
        simdProcessor->MemcpyStream((byte *)currentBufferSet->mappedUniformBase + offset, (byte *)data, bytes);
#else
        graphics.BindBuffer(Graphics::BufferType::Uniform, currentBufferSet->uniformBuffer);
        void *base = graphics.MapBufferRange(currentBufferSet->uniformBuffer, Graphics::BufferLockMode::WriteOnly, offset, bytes);

        simdProcessor->MemcpyStream((byte *)base, (byte *)data, bytes);

        graphics.UnmapBuffer(currentBufferSet->uniformBuffer);
        graphics.BindBuffer(Graphics::BufferType::Uniform, Graphics::NullBuffer);
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
    int offset = graphics.BufferWrite(currentBufferSet->texelBuffer, TB_BPP, bytes, nullptr);
    if (offset == -1) {
        BE_FATALERROR("Out of texel cache");
        return false;
    }

    currentBufferSet->texelMemUsed.store(offset + bytes);
    currentBufferSet->allocations++;

    if (data) {
#if USE_PINNED_MEMORY
        assert(currentBufferSet->mappedTexelBase);
        simdProcessor->MemcpyStream((byte *)currentBufferSet->mappedTexelBase + offset, (byte *)data, bytes);
#else
        graphics.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
        void *base = graphics.MapBufferRange(currentBufferSet->texelBuffer, Graphics::BufferLockMode::WriteOnly, offset, bytes);

        simdProcessor->MemcpyStream((byte *)base, (byte *)data, bytes);

        graphics.UnmapBuffer(currentBufferSet->texelBuffer);
        graphics.BindBuffer(currentBufferSet->texelBufferType, Graphics::NullBuffer);
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
    graphics.BindBuffer(Graphics::BufferType::Vertex, currentBufferSet->vertexBuffer);
    return (byte *)graphics.MapBufferRange(currentBufferSet->vertexBuffer, Graphics::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedIndexBase + bc->offset;
#else
    graphics.BindBuffer(Graphics::BufferType::Index, currentBufferSet->indexBuffer);
    return (byte *)graphics.MapBufferRange(currentBufferSet->indexBuffer, Graphics::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapUniformBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedUniformBase + bc->offset;
#else
    graphics.BindBuffer(Graphics::BufferType::Uniform, currentBufferSet->uniformBuffer);
    return (byte *)graphics.MapBufferRange(currentBufferSet->uniformBuffer, Graphics::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

byte *BufferCacheManager::MapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
    assert(bc->frameCount == frameCount);
#if USE_PINNED_MEMORY
    return (byte *)currentBufferSet->mappedTexelBase + bc->offset;
#else
    graphics.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    return (byte *)graphics.MapBufferRange(currentBufferSet->texelBuffer, Graphics::BufferLockMode::WriteOnly, bc->offset, bc->bytes);
#endif
}

void BufferCacheManager::UnmapVertexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //graphics.BindBuffer(Graphics::BufferType::Vertex, currentBufferSet->vertexBuffer);
    //graphics.FlushMappedBufferRange(currentBufferSet->vertexBuffer, bc->offset, bc->bytes);
#else
    graphics.UnmapBuffer(currentBufferSet->vertexBuffer);
#endif
}

void BufferCacheManager::UnmapIndexBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //graphics.BindBuffer(Graphics::BufferType::Index, currentBufferSet->indexBuffer);
    //graphics.FlushMappedBufferRange(currentBufferSet->indexBuffer, bc->offset, bc->bytes);
#else
    graphics.UnmapBuffer(currentBufferSet->indexBuffer);
#endif
}

void BufferCacheManager::UnmapUniformBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //graphics.BindBuffer(Graphics::BufferType::Uniform, currentBufferSet->uniformBuffer);
    //graphics.FlushMappedBufferRange(currentBufferSet->uniformBuffer, bc->offset, bc->bytes);
#else
    graphics.UnmapBuffer(currentBufferSet->uniformBuffer);
#endif
}

void BufferCacheManager::UnmapTexelBuffer(BufferCache *bc) const {
    const FrameDataBufferSet *currentBufferSet = &frameData[mappedNum];
#if USE_PINNED_MEMORY
    //graphics.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);
    //graphics.FlushMappedBufferRange(currentBufferSet->texelBuffer, bc->offset, bc->bytes);
#else
    graphics.UnmapBuffer(currentBufferSet->texelBuffer);
#endif
}

bool BufferCacheManager::IsCached(const BufferCache *bc) const {
    if (bc->frameCount == 0xFFFFFFFF) { // static buffer
        return true;
    }

    if (bc->buffer != Graphics::NullBuffer && bc->frameCount == frameCount) {
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

void BufferCacheManager::UpdatePBOTexture() {
    BE_PROFILE_CPU_SCOPE_STATIC("BufferCacheManager::UpdatePBOTexture");

    const FrameDataBufferSet *currentBufferSet = &frameData[unmappedNum];

    int texelOffset = pboWriteOffset / TB_BPP;

    pboWriteOffset = 0;

    int updateW = Min(texelOffset, TB_WIDTH);
    int updateH = (texelOffset + TB_WIDTH - 1) / TB_WIDTH;

    if (updateW * updateH > 0) {
        frameData[0].texture->Bind();

        // Transfer PBO -> texture using DMA.
        // If asynchronous DMA transfer is supported, glTexSubImage2D() should return immediately.
        graphics.BindBuffer(currentBufferSet->texelBufferType, currentBufferSet->texelBuffer);

        const double startUpdatePBO = PlatformTime::Seconds();

        frameData[0].texture->Update2D(0, 0, 0, updateW, updateH, Image::Format::R32G32B32A32_FLOAT, nullptr);

        const double endUpdatePBO = PlatformTime::Seconds();
        if (endUpdatePBO - startUpdatePBO > 1) {
            BE_DLOG("BufferCacheManager::UpdatePBOTexture: update pbo took %.3f msec\n", endUpdatePBO - startUpdatePBO);
        }

        graphics.BindBuffer(currentBufferSet->texelBufferType, Graphics::NullBuffer);
    }
}

const Texture *BufferCacheManager::GetFrameTexture() const {
    if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Pbo) {
        return frameData[0].texture;
    }
    return frameData[unmappedNum].texture;
}

BE_NAMESPACE_END
