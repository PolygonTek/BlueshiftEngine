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

class D3D12Texture {
public:
    ~D3D12Texture() { Release(); }

    void                            Release();

    static D3D12Texture *           CreateTexture2D(const char* filename, bool useCompression = true, bool useNormalMap = false);
    static D3D12Texture *           CreateTexture2D(const Image* image, Image::Format::Enum dstFormat, bool useMipmaps);
    static D3D12Texture *           CreateTexture2D(const Image* image);

    static bool                     ImageFormatToDXGIFormat(Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT* dxgiFormat);
    static bool                     IsSupportedImageFormat(Image::Format::Enum imageFormat) { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    static Image::Format::Enum      ToUncompressedImageFormat(Image::Format::Enum imageFormat);
    static Image::Format::Enum      ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap);
    static void                     AdjustTextureFormat(bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           textureAllocation = nullptr;
#else
    ID3D12Resource *                textureResource = nullptr;
#endif
    D3D12_RESOURCE_DESC             textureDesc;
    D3D12_CPU_DESCRIPTOR_HANDLE *   descriptorHandlePtr = nullptr;
};
