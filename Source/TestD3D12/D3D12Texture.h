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

class D3D12Texture {
public:
    ~D3D12Texture() { Release(); }

    void                            Release();

    static D3D12Texture *           CreateTexture2D(const char *filename, bool useCompression = true, bool useNormalMap = false);
    static D3D12Texture *           CreateTexture2D(const BE1::Image *image, BE1::Image::Format::Enum dstFormat, bool useMipmaps);
    static D3D12Texture *           CreateTexture2D(const BE1::Image *image);

    static bool                     ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT* dxgiFormat);
    static bool                     IsSupportedImageFormat(BE1::Image::Format::Enum imageFormat) { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    static BE1::Image::Format::Enum ToUncompressedImageFormat(BE1::Image::Format::Enum imageFormat);
    static BE1::Image::Format::Enum ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap);
    static void                     AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat);

    ID3D12Resource *                textureResource = nullptr;
    D3D12_RESOURCE_DESC             textureDesc;
};
