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

#include "../RHI.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

class D3D12Renderer;

class D3D12Texture : public RHI::Texture {
    friend class D3D12Renderer;

public:
    virtual ~D3D12Texture() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeTextureObject() const override { return this; }

    virtual bool                    IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const override;

    virtual uint32_t                GetWidth() const override { return textureDesc.Width; }
    virtual uint32_t                GetHeight() const override { return textureDesc.Height; }
    virtual uint32_t                GetDepth() const override { return textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? textureDesc.DepthOrArraySize : 1; }
    virtual uint32_t                GetArraySize() const override { return textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : textureDesc.DepthOrArraySize; }
    virtual uint32_t                GetMipLevelCount() const override { return textureDesc.MipLevels; }

    ID3D12Resource *                GetResource() const;

private:
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           textureAllocation = nullptr;
#else
    ID3D12Resource *                textureResource = nullptr;
#endif
    BE1::Image::Format              srcFormat = BE1::Image::Format::Unknown;
    bool                            srcSRGB = false;
    D3D12_RESOURCE_DESC             textureDesc;
    D3D12_CLEAR_VALUE               clearValue;
    RHI::GPUResourceState           initialState = RHI::GPUResourceState::Undefined;
    D3D12SRVDescriptor              srvDescriptor;
    D3D12UAVDescriptor              uavDescriptor;
    D3D12RTVDescriptor              rtvDescriptor;
    D3D12DSVDescriptor              dsvDescriptor;
    BE1::Array<D3D12SRVDescriptor>  subresourceSrvDescriptors = (16);
    BE1::Array<D3D12UAVDescriptor>  subresourceUavDescriptors = (16);
    BE1::Array<D3D12RTVDescriptor>  subresourceRtvDescriptors = (16);
    BE1::Array<D3D12DSVDescriptor>  subresourceDsvDescriptors = (16);
};

BE_INLINE ID3D12Resource *D3D12Texture::GetResource() const {
#ifdef USE_D3D12_MEMALLOC
    return textureAllocation->GetResource();
#else
    return textureResource;
#endif
}
