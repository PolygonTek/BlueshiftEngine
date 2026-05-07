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

#include "../RHI.h"
#include "D3D12Common.h"

class D3D12Texture;
class D3D12FrameThreadData;
class D3D12CommandList;

struct D3D12UploadBufferRequest {
    const void *                        data = nullptr;
    uint64_t                            dataSize = 0;
    D3D12_HEAP_TYPE                     heapType;
    D3D12_RESOURCE_DESC                 resourceDesc = {};
    D3D12_RESOURCE_STATES               initialState = D3D12_RESOURCE_STATE_COMMON;
};

struct D3D12UploadTextureRequest {
    std::shared_ptr<const BE1::Image>   srcImage;
    D3D12_RESOURCE_DESC                 resourceDesc = {};
    D3D12_RESOURCE_STATES               initialState = D3D12_RESOURCE_STATE_COMMON;
    bool                                allocateEmptyMipmaps = false;
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

struct D3D12ResourceRequest {
    enum class Type : uint8_t {
        None,
        DeleteGPUObject,
        UploadBuffer,
        UploadTexture,
        UploadTextureSubImage,
        GenerateMipmaps
    };

    Type                                type = Type::None;
    RHI::GPUObject *                    object = nullptr;
    D3D12UploadBufferRequest            uploadBufferRequest;
    D3D12UploadTextureRequest           uploadTextureRequest;
    D3D12UploadTextureSubImageRequest   uploadTextureSubImageRequest;
};

struct D3D12ResourceCommandContext {
    D3D12FrameThreadData *              frameThreadData = nullptr;
    D3D12CommandList *                  graphicsCommandList = nullptr;
    D3D12CommandList *                  computeCommandList = nullptr;

    bool                                IsGraphicsCommandListUsed() const { return graphicsCommandList != nullptr; }
    D3D12CommandList *                  GetGraphicsCommandList();

    bool                                IsComputeCommandListUsed() const { return computeCommandList != nullptr; }
    D3D12CommandList *                  GetComputeCommandList();
};
