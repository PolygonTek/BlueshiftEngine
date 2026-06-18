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

#include "Precompiled.h"
#include "Image/Image.h"
#include "Image/BCEncoder.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

void CompressBC1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = BCEncoder::CompressImageBC1HQ;
    } else {
        CompressImage = BCEncoder::CompressImageBC1Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressBC2(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = BCEncoder::CompressImageBC2HQ;
    } else {
        CompressImage = BCEncoder::CompressImageBC2Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressBC3(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = BCEncoder::CompressImageBC3HQ;
    } else {
        CompressImage = BCEncoder::CompressImageBC3Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressBC4(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = BCEncoder::CompressImageBC4HQ;
    } else {
        CompressImage = BCEncoder::CompressImageBC4Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

void CompressBC5(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8);

    void (*CompressImage)(const byte *src, const int width, const int height, const int depth, byte *dst);

    if (compressionQuality == Image::CompressionQuality::HighQuality) {
        CompressImage = BCEncoder::CompressImageBC5HQ;
    } else {
        CompressImage = BCEncoder::CompressImageBC5Fast;
    }

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            CompressImage(src, w, h, d, dst);
        }
    }
}

BE_NAMESPACE_END
