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
