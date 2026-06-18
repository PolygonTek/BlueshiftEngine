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
#include "Image/BCDecoder.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

void DecompressBC1(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R8G8B8A8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            BCDecoder::DecompressImageBC1(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC2(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R8G8B8A8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            BCDecoder::DecompressImageBC2(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC3(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R8G8B8A8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            BCDecoder::DecompressImageBC3(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC4(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R8G8B8A8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            BCDecoder::DecompressImageBC4(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC5(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R8G8B8A8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            BCDecoder::DecompressImageBC5(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC6H_UF16(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R32G32B32A32_FLOAT);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            float *dst = reinterpret_cast<float *>(dstImage.GetPixels(mipLevel, sliceIndex));

            BCDecoder::DecompressImageBC6HU(srcBlocks, w, h, d, dst);
        }
    }
}

void DecompressBC6H_SF16(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::R32G32B32A32_FLOAT);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            int w = srcImage.GetWidth(mipLevel);
            int h = srcImage.GetHeight(mipLevel);
            int d = srcImage.GetDepth(mipLevel);

            const byte *srcBlocks = srcImage.GetPixels(mipLevel, sliceIndex);
            float *dst = reinterpret_cast<float *>(dstImage.GetPixels(mipLevel, sliceIndex));

            BCDecoder::DecompressImageBC6HS(srcBlocks, w, h, d, dst);
        }
    }
}

BE_NAMESPACE_END
