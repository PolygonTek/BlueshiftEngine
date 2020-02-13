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
#include "Core/Str.h"
#include "Core/Heap.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

static bool DecompressImage(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetPixels());

    switch (srcImage.GetFormat()) {
    case Image::Format::DXT1:
        DecompressDXT1(srcImage, dstImage);
        break;
    case Image::Format::DXT3:
        DecompressDXT3(srcImage, dstImage);
        break;
    case Image::Format::DXT5:
        DecompressDXT5(srcImage, dstImage);
        break;
    case Image::Format::XGBR_DXT5:
        DecompressDXT5(srcImage, dstImage);
        BE_WARNLOG("DecompressImage: XGBR_DXT5 need to swap channels\n");
        break;
    case Image::Format::DXN2:
        DecompressDXN2(srcImage, dstImage);
        break;
    case Image::Format::RGB_PVRTC_2BPPV1:
    case Image::Format::RGBA_PVRTC_2BPPV1:
    case Image::Format::RGBA_PVRTC_2BPPV2:
        DecompressPVRTC(srcImage, dstImage, 1);
        break;
    case Image::Format::RGB_PVRTC_4BPPV1:
    case Image::Format::RGBA_PVRTC_4BPPV1:
    case Image::Format::RGBA_PVRTC_4BPPV2:
        DecompressPVRTC(srcImage, dstImage, 0);
        break;
    case Image::Format::RGB_8_ETC1:
        DecompressETC1(srcImage, dstImage);
        break;
    case Image::Format::RGB_8_ETC2:
        DecompressETC2_RGB8(srcImage, dstImage);
        break;
    case Image::Format::RGBA_8_1_ETC2:
        DecompressETC2_RGB8A1(srcImage, dstImage);
        break;
    case Image::Format::RGBA_8_8_ETC2:
        DecompressETC2_RGBA8(srcImage, dstImage);
        break;
    case Image::Format::R_11_EAC:
        DecompressEAC_R11(srcImage, dstImage, false);
        break;
    case Image::Format::SignedR_11_EAC:
        DecompressEAC_R11(srcImage, dstImage, true);
        break;
    case Image::Format::RG_11_11_EAC:
        // Consider RG_11_11_EAC format image as normal values.
        DecompressEAC_RG11(srcImage, dstImage, false, true);
        break;
    case Image::Format::SignedRG_11_11_EAC:
        // Consider SignedRG_11_11_EAC format image as normal values.
        DecompressEAC_RG11(srcImage, dstImage, true, true);
        break;
    default:
        BE_WARNLOG("DecompressImage: unsupported format %s\n", srcImage.FormatName());
        return false;
    }

    return true;
}

static bool CompressImage(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality) {
    assert(srcImage.GetPixels());

    //uint64_t startClocks = rdtsc();

    switch (dstImage.GetFormat()) {
    case Image::Format::DXT1:
        CompressDXT1(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::DXT3:
        CompressDXT3(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::DXT5:
        CompressDXT5(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::DXN2:
        CompressDXN2(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::RGB_8_ETC1:
        CompressETC1(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::RGB_8_ETC2:
        CompressETC2_RGB8(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::RGBA_8_1_ETC2:
        CompressETC2_RGBA1(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::RGBA_8_8_ETC2:
        CompressETC2_RGBA8(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::R_11_EAC:
        CompressEAC_R11(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::SignedR_11_EAC:
        CompressEAC_Signed_R11(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::RG_11_11_EAC:
        CompressEAC_RG11(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::SignedRG_11_11_EAC:
        CompressEAC_Signed_RG11(srcImage, dstImage, compressionQuality);
        break;
    default:
        BE_WARNLOG("CompressImage: unsupported format %s\n", dstImage.FormatName());
        return false;
    }

    //uint64_t endClocks = rdtsc();
    //BE_LOG("%DXT compression in %" PRIu64 " clocks\n", endClocks - startClocks);

    return true;
}

static void SRGBToLinear(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::GammaToLinearApprox(*dataPtr);
    }
}

static void Pow22ToLinear(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::GammaToLinearFast(*dataPtr);
    }
}

static void LinearToSRGB(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::LinearToGammaApprox(*dataPtr);
    }
}

static void LinearToPow22(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::LinearToGammaFast(*dataPtr);
    }
}

static void SRGBToPow22(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::LinearToGammaFast(Image::GammaToLinearApprox(*dataPtr));
    }
}

static void Pow22ToSRGB(float *data, int count) {
    float *dataPtr = data;
    float *dataEnd = data + count;

    while (dataPtr < dataEnd) {
        *dataPtr++ = Image::LinearToGammaApprox(Image::GammaToLinearFast(*dataPtr));
    }
}

using GammaConversionFunc = void (*)(float *data, int count);

static GammaConversionFunc GetGammaConversionFunc(Image::GammaSpace::Enum srcGammaSpace, Image::GammaSpace::Enum dstGammaSpace) {
    if (srcGammaSpace == dstGammaSpace) {
        return nullptr;
    }

    if (srcGammaSpace == Image::GammaSpace::Linear) {
        if (dstGammaSpace == Image::GammaSpace::sRGB) {
            return LinearToSRGB;
        }
        return LinearToPow22;
    }
    
    if (dstGammaSpace == Image::GammaSpace::Linear) {
        if (srcGammaSpace == Image::GammaSpace::sRGB) {
            return SRGBToLinear;
        }
        return Pow22ToLinear;
    }

    if (srcGammaSpace == Image::GammaSpace::sRGB) {
        return SRGBToPow22;
    }
    return Pow22ToSRGB;
}

bool Image::ConvertFormat(Image::Format::Enum dstFormat, Image &dstImage, GammaSpace::Enum dstGammaSpace, bool regenerateMipmaps, Image::CompressionQuality::Enum compressionQuality) const {
    if (dstGammaSpace == GammaSpace::DontCare) {
        dstGammaSpace = gammaSpace;
    }

    // If the source and destination formats are the same, copy the source image to the destination image and return.
    if (format == dstFormat && gammaSpace == dstGammaSpace) {
        dstImage = *this;
        return true;
    }

    const Image *srcImage = this;

    // Calculate the mipmap count for the destination image.
    int numDstMipmaps = regenerateMipmaps ? MaxMipMapLevels(width, height, depth) : numMipmaps;

    // Create a destination image based on the source (this) image.
    dstImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->numSlices, numDstMipmaps, dstFormat, dstGammaSpace, nullptr, srcImage->flags);

    Image unpackedSrcImage;

    if (srcImage->IsCompressed()) {
        // If the source image is compressed, decompress it first.
        unpackedSrcImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->numSlices, numDstMipmaps,
            srcImage->NeedFloatConversion() ? Format::RGBA_32F_32F_32F_32F : Format::RGBA_8_8_8_8, srcImage->gammaSpace, nullptr, srcImage->flags);
        
        DecompressImage(*this, unpackedSrcImage);

        if (regenerateMipmaps) {
            unpackedSrcImage.GenerateMipmaps();
        }

        srcImage = &unpackedSrcImage;
    } else if (regenerateMipmaps) {
        if (!srcImage->IsPacked()) {
            unpackedSrcImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->numSlices, numDstMipmaps,
                srcImage->format, srcImage->gammaSpace, nullptr, srcImage->flags);

            // Copy only the first level of the source image for mipmap generation.
            unpackedSrcImage.CopyFrom(*srcImage, 0, 1);
        } else {
            // Packed format can't generate mipmaps directly, so we need to unpack it.
            srcImage->ConvertFormat(Format::RGBA_8_8_8_8, unpackedSrcImage, dstGammaSpace);
        }

        unpackedSrcImage.GenerateMipmaps();

        srcImage = &unpackedSrcImage;
    }

    // If the destination format is compressed, we need to convert source image to RGBA_8_8_8_8 or RGBA_32F_32F_32F_32F first.
    if (Image::IsCompressed(dstFormat)) {
        Image tempImage;

        if (Image::NeedFloatConversion(dstFormat) || srcImage->gammaSpace != dstGammaSpace) {
            if (srcImage->GetFormat() != Format::RGBA_32F_32F_32F_32F) {
                srcImage->ConvertFormat(Format::RGBA_32F_32F_32F_32F, tempImage, dstGammaSpace);
                srcImage = &tempImage;
            }
        } else {
            if (srcImage->GetFormat() != Format::RGBA_8_8_8_8) {
                srcImage->ConvertFormat(Format::RGBA_8_8_8_8, tempImage);
                srcImage = &tempImage;
            }
        }

        if (!CompressImage(*srcImage, dstImage, compressionQuality)) {
            dstImage.Clear();
            return false;
        }
        return true;
    }

    ImageUnpackFunc unpackFunc;
    ImagePackFunc packFunc;
    int componentSize;

    const ImageFormatInfo *srcFormatInfo = GetImageFormatInfo(srcImage->GetFormat());
    const ImageFormatInfo *dstFormatInfo = GetImageFormatInfo(dstFormat);

    GammaConversionFunc gammaConversionFunc = GetGammaConversionFunc(srcImage->gammaSpace, dstGammaSpace);

    if (gammaConversionFunc || NeedFloatConversion(srcImage->GetFormat()) || NeedFloatConversion(dstFormat)) {
        unpackFunc = srcFormatInfo->unpackRGBA32F;
        packFunc = dstFormatInfo->packRGBA32F;
        componentSize = sizeof(float);
    } else {
        unpackFunc = srcFormatInfo->unpackRGBA8888;
        packFunc = dstFormatInfo->packRGBA8888;
        componentSize = sizeof(byte);
    }

    if (!unpackFunc || !packFunc) {
        BE_WARNLOG("Image::ConvertFormat: unsupported convert type (from %s to %s)\n", srcImage->FormatName(), dstImage.FormatName());
        dstImage.Clear();
        return false;
    }

    byte *unpackedBuffer = (byte *)Mem_Alloc16(width * 4 * componentSize);

    byte *srcPtr = srcImage->GetPixels();
    byte *dstPtr = dstImage.GetPixels();

    for (int mipLevel = 0; mipLevel < srcImage->numMipmaps; mipLevel++) {
        int w = srcImage->GetWidth(mipLevel);
        int h = srcImage->GetHeight(mipLevel);
        int d = srcImage->GetDepth(mipLevel);

        int srcPitch = srcImage->BytesPerPixel() * w;
        int dstPitch = dstImage.BytesPerPixel() * w;

        for (int sliceIndex = 0; sliceIndex < srcImage->numSlices; sliceIndex++) {
            for (int z = 0; z < d; z++) {
                for (int y = 0; y < h; y++) {
                    unpackFunc(srcPtr, unpackedBuffer, w);

                    if (gammaConversionFunc) {
                        gammaConversionFunc((float *)unpackedBuffer, 4 * w);
                    }

                    packFunc(unpackedBuffer, dstPtr, w);

                    srcPtr += srcPitch;
                    dstPtr += dstPitch;
                }
            }
        }
    }

    Mem_AlignedFree(unpackedBuffer);
    return true;
}

bool Image::ConvertFormatSelf(Image::Format::Enum dstFormat, GammaSpace::Enum dstGammaSpace, bool regenerateMipmaps, Image::CompressionQuality::Enum compressionQuality) {
    Image dstImage;
    bool ret = ConvertFormat(dstFormat, dstImage, dstGammaSpace, regenerateMipmaps, compressionQuality);
    if (ret) {
        *this = dstImage;
    }
    return ret;
}

BE_NAMESPACE_END
