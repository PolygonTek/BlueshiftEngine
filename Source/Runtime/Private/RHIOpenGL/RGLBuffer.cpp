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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

static const GLenum ToGLBufferUsage(RHI::BufferUsage::Enum usage) {
    switch (usage) {
    case RHI::BufferUsage::Static:
        return GL_STATIC_DRAW;
    case RHI::BufferUsage::Dynamic:
        return GL_DYNAMIC_DRAW;
    case RHI::BufferUsage::Stream:
        return GL_STREAM_DRAW;
    default:
        assert(0);
        return 0;
    }
}

static const GLenum ToGLBufferTarget(RHI::BufferType::Enum type) {
    switch (type) {
    case RHI::BufferType::Vertex:
        return GL_ARRAY_BUFFER;
    case RHI::BufferType::Index:
        return GL_ELEMENT_ARRAY_BUFFER;
    case RHI::BufferType::PixelPack:
        return GL_PIXEL_PACK_BUFFER;
    case RHI::BufferType::PixelUnpack:
        return GL_PIXEL_UNPACK_BUFFER;
    case RHI::BufferType::Texel:
        return GL_TEXTURE_BUFFER;
    case RHI::BufferType::Uniform:
        return GL_UNIFORM_BUFFER;
    case RHI::BufferType::TransformFeedback:
        return GL_TRANSFORM_FEEDBACK_BUFFER;
    case RHI::BufferType::CopyRead:
        return GL_COPY_READ_BUFFER;
#ifdef GL_ARB_draw_indirect
    case RHI::BufferType::DrawIndirect:
        return GL_DRAW_INDIRECT_BUFFER;
#endif
    default:
        assert(0);
        return 0;
    }
}

RHI::Handle OpenGLRHI::CreateBuffer(BufferType::Enum type, BufferUsage::Enum usage, int size, int pitch, const void *data) {
    GLenum target = ToGLBufferTarget(type);

    GLBuffer *buffer = new GLBuffer;
    buffer->type = type;
    buffer->usage = ToGLBufferUsage(usage);
    buffer->target = target;
    buffer->size = size;
    buffer->pitch = pitch;
    buffer->writeOffset = 0;
    buffer->bindingIndex = -1;
    buffer->bindingOffset = 0;
    buffer->bindingSize = 0;

    gglGenBuffers(1, &buffer->object);

    if (size > 0) {
        gglBindBuffer(target, buffer->object);
        gglBufferData(target, size, data, buffer->usage);
    }

    int handle = bufferList.FindNull();
    if (handle == -1) {
        handle = bufferList.Append(buffer);
    } else {
        bufferList[handle] = buffer;
    }

    if (size > 0) {
        Handle *bufferHandlePtr = &currentContext->state->bufferHandles[type];
        *bufferHandlePtr = handle;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyBuffer(Handle bufferHandle) {
    GLBuffer *buffer = bufferList[bufferHandle];

    for (int i = 0; i < COUNT_OF(currentContext->state->bufferHandles); i++) {
        if (bufferHandle == currentContext->state->bufferHandles[i]) {
            currentContext->state->bufferHandles[i] = NullBuffer;
            break;
        }
    }

    // If a buffer object that is currently bound is deleted, the binding reverts to 0.
    gglDeleteBuffers(1, &buffer->object);

    delete bufferList[bufferHandle];
    bufferList[bufferHandle] = nullptr;
}

void OpenGLRHI::BindBuffer(BufferType::Enum type, Handle bufferHandle) {
    Handle *bufferHandlePtr = &currentContext->state->bufferHandles[type];
    if (*bufferHandlePtr != bufferHandle) {
        *bufferHandlePtr = bufferHandle;
        const GLBuffer *buffer = bufferList[bufferHandle];
        gglBindBuffer(ToGLBufferTarget(type), buffer->object);
    }
}

void OpenGLRHI::BindIndexedBuffer(BufferType::Enum type, int bindingIndex, Handle bufferHandle) {
    // Allowed only target UniformBuffer or TransformFeedbackBuffer.
    assert(type == BufferType::Uniform || type == BufferType::TransformFeedback);
    int targetIndex = type - BufferType::Uniform;
    Handle *bufferHandlePtr = &currentContext->state->indexedBufferHandles[targetIndex];
    GLBuffer *buffer = bufferList[bufferHandle];
    if (*bufferHandlePtr != bufferHandle || buffer->bindingIndex != bindingIndex || buffer->bindingOffset != 0 || buffer->bindingSize != buffer->size) {
        *bufferHandlePtr = bufferHandle;
        buffer->bindingIndex = bindingIndex;
        buffer->bindingOffset = 0;
        buffer->bindingSize = buffer->size;
        gglBindBufferBase(ToGLBufferTarget(type), bindingIndex, buffer->object);
    }
}

void OpenGLRHI::BindIndexedBufferRange(BufferType::Enum type, int bindingIndex, Handle bufferHandle, int offset, int size) {
    // Allowed only target UniformBuffer or TransformFeedbackBuffer.
    assert(type == BufferType::Uniform || type == BufferType::TransformFeedback);
    int targetIndex = type - BufferType::Uniform;
    Handle *bufferHandlePtr = &currentContext->state->indexedBufferHandles[targetIndex];
    GLBuffer *buffer = bufferList[bufferHandle];
    if (*bufferHandlePtr != bufferHandle || buffer->bindingIndex != bindingIndex || buffer->bindingOffset != offset || buffer->bindingSize != size) {
        *bufferHandlePtr = bufferHandle;
        buffer->bindingIndex = bindingIndex;
        buffer->bindingOffset = offset;
        buffer->bindingSize = size;
        gglBindBufferRange(ToGLBufferTarget(type), bindingIndex, buffer->object, offset, size);
    }
}

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
// GL_MAP_INVALIDATE_RANGE_BIT -- previous contents of the specified range may be discarded
// GL_MAP_UNSYNCHRONIZED_BIT -- GL should not attempt to synchronize pending operations on the buffer prior to returning from glMapBufferRange
// GL_MAP_FLUSH_EXPLICIT_BIT -- modifications to each subrange must be explicitly flushed by calling glFlushMappedBufferRange()
// GL_MAP_PERSISTENT_BIT -- keep mapping and that the client intends to hold and use the returned pointer during subsequent GL operation
// GL_MAP_COHERENT_BIT -- persistent mapping is also to be coherent (automatically visible to GPU)
void *OpenGLRHI::MapBufferRange(Handle bufferHandle, BufferLockMode::Enum lockMode, int offset, int size) {
    GLBuffer *buffer = bufferList[bufferHandle];
    
    if (size < 0) {
        size = buffer->size;
    }

    assert(offset + size <= buffer->size);

    GLbitfield access = 0;

    switch (lockMode) {
    case BufferLockMode::WriteOnly:
        access |= GL_MAP_WRITE_BIT;
        break;
    case BufferLockMode::WriteOnlyExplicitFlush:
        access |= (GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
        break;
    case BufferLockMode::WriteOnlyPersistent:
        access |= GL_MAP_WRITE_BIT;
#ifdef GL_ARB_buffer_storage
        if (OpenGL::SupportsBufferStorage()) {
            access |= GL_MAP_PERSISTENT_BIT;

            gglBufferStorage(buffer->target, size, 0, access);
        }
#endif
        break;
    }

    void *ptr = nullptr;
    if (offset > 0 || offset + size < buffer->size) {
        ptr = gglMapBufferRange(buffer->target, offset, size, access | GL_MAP_UNSYNCHRONIZED_BIT);
    } else {
        ptr = gglMapBufferRange(buffer->target, 0, size, access);
    }

    return ptr;
}

bool OpenGLRHI::UnmapBuffer(Handle bufferHandle) {
    GLBuffer *buffer = bufferList[bufferHandle];
    // glUnmapBuffer returns GL_TRUE unless the data store contents have become corrupt during the time the data
    // store was mapped. This can occur for system-specific reasons that affect the availability of graphics
    // memory, such as screen mode changes. In such situations, GL_FALSE is returned and the data store contents
    // are undefined. An application must detect this rare condition and reinitialize the data store.
    return !!gglUnmapBuffer(buffer->target);
}

void OpenGLRHI::FlushMappedBufferRange(Handle bufferHandle, int offset, int size) {
    GLBuffer *buffer = bufferList[bufferHandle];

    if (size < 0) {
        size = buffer->size;
    }

    assert(offset + size <= buffer->size);

    gglFlushMappedBufferRange(buffer->target, offset, size);
}

void OpenGLRHI::WriteBuffer(byte *dst, const byte *src, int numBytes) {
    assert_16_byte_aligned(dst);
    assert_16_byte_aligned(src);

#if defined(ENABLE_SIMD4_INTRIN)
    int i = 0;
    for (; i + 128 <= numBytes; i += 128) {
        simd4i d0 = load_si128((int32_t *)&src[i + 0 * 16]);
        simd4i d1 = load_si128((int32_t *)&src[i + 1 * 16]);
        simd4i d2 = load_si128((int32_t *)&src[i + 2 * 16]);
        simd4i d3 = load_si128((int32_t *)&src[i + 3 * 16]);
        simd4i d4 = load_si128((int32_t *)&src[i + 4 * 16]);
        simd4i d5 = load_si128((int32_t *)&src[i + 5 * 16]);
        simd4i d6 = load_si128((int32_t *)&src[i + 6 * 16]);
        simd4i d7 = load_si128((int32_t *)&src[i + 7 * 16]);

        storent_si128(d0, (int32_t *)&dst[i + 0 * 16]);
        storent_si128(d1, (int32_t *)&dst[i + 1 * 16]);
        storent_si128(d2, (int32_t *)&dst[i + 2 * 16]);
        storent_si128(d3, (int32_t *)&dst[i + 3 * 16]);
        storent_si128(d4, (int32_t *)&dst[i + 4 * 16]);
        storent_si128(d5, (int32_t *)&dst[i + 5 * 16]);
        storent_si128(d6, (int32_t *)&dst[i + 6 * 16]);
        storent_si128(d7, (int32_t *)&dst[i + 7 * 16]);
    }
    for (; i + 16 <= numBytes; i += 16) {
        simd4i d = load_si128((int32_t *)&src[i]);
        storent_si128(d, (int32_t *)&dst[i]);
    }
    for (; i + 4 <= numBytes; i += 4) {
        *(uint32_t *)&dst[i] = *(const uint32_t *)&src[i];
    }
    for (; i < numBytes; i++) {
        dst[i] = src[i];
    }
    sfence();
#else
    memcpy(dst, src, numBytes);
#endif
}

int OpenGLRHI::BufferDiscardWrite(Handle bufferHandle, int size, const void *data) {
    GLBuffer *buffer = bufferList[bufferHandle];

    if (gglMapBufferRange) {
        // NOTE: glMapBufferRange() function causes GL_INVALID_VALUE error if buffer is not alloced.
        gglBufferData(buffer->target, size, nullptr, buffer->usage);
        byte *dest = (byte *)gglMapBufferRange(buffer->target, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        if (!((intptr_t)data & 15)) {
            WriteBuffer(dest, (const byte *)data, size);
        } else {
            memcpy(dest, data, size);
        }
        gglUnmapBuffer(buffer->target);
    } else {
        // Do buffer respecification using glBufferData().
        gglBufferData(buffer->target, size, nullptr, buffer->usage);
        gglBufferData(buffer->target, size, data, buffer->usage);
    }

    buffer->size = size;
    buffer->writeOffset = 0;

    return 0;
}

int OpenGLRHI::BufferWrite(Handle bufferHandle, int alignSize, int size, const void *data) {
    GLBuffer *writeBuffer = bufferList[bufferHandle];

    if (writeBuffer->pitch > 0 && size > writeBuffer->pitch) {
        return -1;
    }

    int base = writeBuffer->writeOffset + alignSize - 1;
    base -= base % alignSize;

    if (writeBuffer->pitch > 0) {
        int startRow = base / writeBuffer->pitch;
        int endRow = (base + size) / writeBuffer->pitch;
        
        if (endRow > startRow) {
            base -= base % writeBuffer->pitch;
            base += writeBuffer->pitch;
        }
    }
    
    int endPos = base + size;

    if (endPos > writeBuffer->size) {
        return -1;
    }

    // If date == nullptr, buffer memory is reserved
    if (data) {
        /*if (gglMapBufferRange) {
            byte *dest = (byte *)gglMapBufferRange(buffer->target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            WriteBuffer(dest, (const byte *)data, size);
            gglUnmapBuffer(buffer->target);
        }*/
        gglBufferSubData(writeBuffer->target, base, size, data);
    }
    
    writeBuffer->writeOffset = endPos;

    return base;
}

int	OpenGLRHI::BufferCopy(Handle readBufferHandle, Handle writeBufferHandle, int alignSize, int size) {
    GLBuffer *writeBuffer = bufferList[writeBufferHandle];
    const GLBuffer *readBuffer = bufferList[readBufferHandle];

    if (writeBuffer->pitch > 0 && size > writeBuffer->pitch) {
        return -1;
    }

    int base = writeBuffer->writeOffset + alignSize - 1;
    base -= base % alignSize;
    
    if (writeBuffer->pitch > 0) {
        int startRow = base / writeBuffer->pitch;
        int endRow = (base + size) / writeBuffer->pitch;
        
        if (endRow > startRow) {
            base -= base % writeBuffer->pitch;
            base += writeBuffer->pitch;
        }
    }
    
    int endPos = base + size;

    if (endPos > writeBuffer->size) {
        return -1;
    }

    gglBindBuffer(GL_COPY_READ_BUFFER, readBuffer->object);
    gglBindBuffer(GL_COPY_WRITE_BUFFER, writeBuffer->object);
    gglCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, base, size);
    gglBindBuffer(GL_COPY_READ_BUFFER, 0);
    gglBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    
    writeBuffer->writeOffset = endPos;

    return base;
}

void OpenGLRHI::BufferRewind(Handle bufferHandle) {
    GLBuffer *buffer = bufferList[bufferHandle];
    
    buffer->writeOffset = 0;
}

BE_NAMESPACE_END
