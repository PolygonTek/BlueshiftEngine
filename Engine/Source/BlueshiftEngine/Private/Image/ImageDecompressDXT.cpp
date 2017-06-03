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
#include "Image/DxtDecoder.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

void DecompressDXT1(const Image &srcImage, Image &dstImage) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const DXTBlock *srcDxtBlock = (const DXTBlock *)srcImage.GetPixels(mipLevel, sliceIndex);

            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DXTDecoder::DecompressImageDXT1(srcDxtBlock, w, h, d, dst);
        }
    }
}

void DecompressDXT3(const Image &srcImage, Image &dstImage) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const DXTBlock *srcDxtBlock = (const DXTBlock *)srcImage.GetPixels(mipLevel, sliceIndex);

            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DXTDecoder::DecompressImageDXT3(srcDxtBlock, w, h, d, dst);
        }
    }
}

void DecompressDXT5(const Image &srcImage, Image &dstImage) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const DXTBlock *srcDxtBlock = (const DXTBlock *)srcImage.GetPixels(mipLevel, sliceIndex);

            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DXTDecoder::DecompressImageDXT5(srcDxtBlock, w, h, d, dst);
        }
    }
}

void DecompressDXN2(const Image &srcImage, Image &dstImage) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const DXTBlock *srcDxtBlock = (const DXTBlock *)srcImage.GetPixels(mipLevel, sliceIndex);

            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DXTDecoder::DecompressImageDXN2(srcDxtBlock, w, h, d, dst);
        }
    }
}

BE_NAMESPACE_END
