// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <new>
#include <utility>
#include "../RHI.h"
#include "D3D12Common.h"

class D3D12Texture;
class D3D12FrameThreadData;
class D3D12CommandList;

// Deletion only needs the common GPUObject target, so no additional payload is stored.
struct D3D12DeleteGPUObjectRequest {};

struct D3D12UploadBufferRequest {
    ID3D12Resource *                    uploadBuffer = nullptr;
    uint64_t                            dataSize = 0;
};

struct D3D12UploadBufferSubDataRequest {
    ID3D12Resource *                    uploadBuffer = nullptr;
    uint64_t                            dstOffset = 0;
    uint64_t                            size = 0;
    RHI::GPUResourceState               stateBefore = RHI::GPUResourceState::Undefined;
    RHI::GPUResourceState               stateAfter = RHI::GPUResourceState::Undefined;
};

struct D3D12UploadTextureRequest {
    SharedPtr<const BE1::Image>         srcImage;
    D3D12_RESOURCE_DESC                 resourceDesc = {};
    int                                 dstFirstMip = 0;
    int                                 mipCount = 0;
};

struct D3D12UploadTextureSubImageRequest {
    ID3D12Resource *                    uploadBuffer = nullptr;
    int                                 subresourceIndex = 0;
    int                                 x = 0;
    int                                 y = 0;
    int                                 z = 0;
    int                                 width = 0;
    int                                 height = 0;
    int                                 depth = 0;
};

struct D3D12BuildMipChainRequest {
    SharedPtr<const BE1::Image>         srcImage;
};

struct D3D12ResourceRequest {
    enum class Type : uint8_t {
        None,
        DeleteGPUObject,
        UploadBuffer,
        UploadBufferSubData,
        UploadTexture,
        UploadTextureSubImage,
        BuildMipChain,
    };

    D3D12ResourceRequest() noexcept {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12DeleteGPUObjectRequest) noexcept : type(Type::DeleteGPUObject), object(target) {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12UploadBufferRequest &&payload) noexcept : type(Type::UploadBuffer), object(target), uploadBufferRequest(std::move(payload)) {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12UploadBufferSubDataRequest &&payload) noexcept : type(Type::UploadBufferSubData), object(target), uploadBufferSubDataRequest(std::move(payload)) {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12UploadTextureRequest &&payload) noexcept : type(Type::UploadTexture), object(target), uploadTextureRequest(std::move(payload)) {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12UploadTextureSubImageRequest &&payload) noexcept : type(Type::UploadTextureSubImage), object(target), uploadTextureSubImageRequest(std::move(payload)) {}
    D3D12ResourceRequest(RHI::GPUObject *target, D3D12BuildMipChainRequest &&payload) noexcept : type(Type::BuildMipChain), object(target), buildMipChainRequest(std::move(payload)) {}
    D3D12ResourceRequest(const D3D12ResourceRequest &) = delete;
    D3D12ResourceRequest &operator=(const D3D12ResourceRequest &) = delete;
    D3D12ResourceRequest(D3D12ResourceRequest &&other) noexcept : type(other.type), object(other.object) { MovePayloadFrom(std::move(other)); }
    D3D12ResourceRequest &operator=(D3D12ResourceRequest &&other) noexcept;
    ~D3D12ResourceRequest() noexcept { DestroyPayload(); }

    Type                                GetType() const { return type; }
    RHI::GPUObject *                    GetObject() const { return object; }

private:
    friend class D3D12Renderer;

    void                                DestroyPayload() noexcept;
    void                                MovePayloadFrom(D3D12ResourceRequest &&other) noexcept;

    Type                                type = Type::None;
    RHI::GPUObject *                    object = nullptr;

    union {
        D3D12UploadBufferRequest        uploadBufferRequest;
        D3D12UploadBufferSubDataRequest uploadBufferSubDataRequest;
        D3D12UploadTextureRequest       uploadTextureRequest;
        D3D12UploadTextureSubImageRequest uploadTextureSubImageRequest;
        D3D12BuildMipChainRequest       buildMipChainRequest;
    };
};

BE_INLINE D3D12ResourceRequest &D3D12ResourceRequest::operator=(D3D12ResourceRequest &&other) noexcept {
    if (this != &other) {
        DestroyPayload();
        type = other.type;
        object = other.object;
        MovePayloadFrom(std::move(other));
    }
    return *this;
}

BE_INLINE void D3D12ResourceRequest::DestroyPayload() noexcept {
    // GPU objects and upload buffers are released by the renderer's existing request paths.
    switch (type) {
    case Type::UploadBuffer:
        uploadBufferRequest.~D3D12UploadBufferRequest();
        break;
    case Type::UploadBufferSubData:
        uploadBufferSubDataRequest.~D3D12UploadBufferSubDataRequest();
        break;
    case Type::UploadTexture:
        uploadTextureRequest.~D3D12UploadTextureRequest();
        break;
    case Type::UploadTextureSubImage:
        uploadTextureSubImageRequest.~D3D12UploadTextureSubImageRequest();
        break;
    case Type::BuildMipChain:
        buildMipChainRequest.~D3D12BuildMipChainRequest();
        break;
    case Type::None:
    case Type::DeleteGPUObject:
        break;
    }
}

BE_INLINE void D3D12ResourceRequest::MovePayloadFrom(D3D12ResourceRequest &&other) noexcept {
    // The source keeps its active member; moved SharedPtrs are empty and destruct normally.
    switch (type) {
    case Type::UploadBuffer:
        ::new (&uploadBufferRequest) D3D12UploadBufferRequest(std::move(other.uploadBufferRequest));
        break;
    case Type::UploadBufferSubData:
        ::new (&uploadBufferSubDataRequest) D3D12UploadBufferSubDataRequest(std::move(other.uploadBufferSubDataRequest));
        break;
    case Type::UploadTexture:
        ::new (&uploadTextureRequest) D3D12UploadTextureRequest(std::move(other.uploadTextureRequest));
        break;
    case Type::UploadTextureSubImage:
        ::new (&uploadTextureSubImageRequest) D3D12UploadTextureSubImageRequest(std::move(other.uploadTextureSubImageRequest));
        break;
    case Type::BuildMipChain:
        ::new (&buildMipChainRequest) D3D12BuildMipChainRequest(std::move(other.buildMipChainRequest));
        break;
    case Type::None:
    case Type::DeleteGPUObject:
        break;
    }
}

struct D3D12ResourceCommandContext {
    D3D12FrameThreadData *              frameThreadData = nullptr;
    D3D12CommandList *                  graphicsCommandList = nullptr;
    D3D12CommandList *                  computeCommandList = nullptr;

    bool                                IsGraphicsCommandListUsed() const { return graphicsCommandList != nullptr; }
    D3D12CommandList *                  GetGraphicsCommandList();

    bool                                IsComputeCommandListUsed() const { return computeCommandList != nullptr; }
    D3D12CommandList *                  GetComputeCommandList();

    void                                CloseAndExecute();
};
