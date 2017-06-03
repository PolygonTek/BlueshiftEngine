#include "Precompiled.h"
#include "Image/Image.h"
#include "Image/DxtEncoder.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

void CompressDXT1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            if (compressoinQuality == Image::HighQuality) {
                DXTEncoder::CompressImageDXT1HQ(src, w, h, d, dst);
            } else {
                DXTEncoder::CompressImageDXT1Fast(src, w, h, d, dst);
            }
        }
    }
}

void CompressDXT3(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            if (compressoinQuality == Image::HighQuality) {
                DXTEncoder::CompressImageDXT3HQ(src, w, h, d, dst);
            } else {
                DXTEncoder::CompressImageDXT3Fast(src, w, h, d, dst);
            }
        }
    }
}

void CompressDXT5(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            if (compressoinQuality == Image::HighQuality) {
                DXTEncoder::CompressImageDXT5HQ(src, w, h, d, dst);
            } else {
                DXTEncoder::CompressImageDXT5Fast(src, w, h, d, dst);
            }
        }
    }
}

void CompressDXN2(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            if (compressoinQuality == Image::HighQuality) {
                DXTEncoder::CompressImageDXN2HQ(src, w, h, d, dst);
            } else {
                DXTEncoder::CompressImageDXN2Fast(src, w, h, d, dst);
            }
        }
    }
}

BE_NAMESPACE_END
