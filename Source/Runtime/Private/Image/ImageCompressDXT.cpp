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
#include "Image/Image.h"
#include "Image/DxtEncoder.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

void CompressDXT1(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = DXTEncoder::CompressImageDXT1HQ;
    } else {
        CompressImage = DXTEncoder::CompressImageDXT1Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressDXT3(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = DXTEncoder::CompressImageDXT3HQ;
    } else {
        CompressImage = DXTEncoder::CompressImageDXT3Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressDXT5(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = DXTEncoder::CompressImageDXT5HQ;
    } else {
        CompressImage = DXTEncoder::CompressImageDXT5Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressDXN2(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = DXTEncoder::CompressImageDXN2HQ;
    } else {
        CompressImage = DXTEncoder::CompressImageDXN2Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

BE_NAMESPACE_END
