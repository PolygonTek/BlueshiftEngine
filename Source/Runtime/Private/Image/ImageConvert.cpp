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
#include "Core/Str.h"
#include "Core/Heap.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

static bool DecompressImage(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetPixels());

    switch (srcImage.GetFormat()) {
    case Image::Format::BC1:
        DecompressBC1(srcImage, dstImage);
        break;
    case Image::Format::BC2:
        DecompressBC2(srcImage, dstImage);
        break;
    case Image::Format::BC3:
        DecompressBC3(srcImage, dstImage);
        break;
    case Image::Format::BC3XGBR:
        DecompressBC3(srcImage, dstImage);
        dstImage.SwapRedAlphaRGBA8888();
        break;
    case Image::Format::BC4:
        DecompressBC4(srcImage, dstImage);
        break;
    case Image::Format::BC5:
        DecompressBC5(srcImage, dstImage);
        break;
    case Image::Format::BC6H_UF16:
        DecompressBC6H_UF16(srcImage, dstImage);
        break;
    case Image::Format::BC6H_SF16:
        DecompressBC6H_SF16(srcImage, dstImage);
        break;
    case Image::Format::BC7:
        DecompressBC7(srcImage, dstImage);
        break;
    case Image::Format::PVRTC12:
    case Image::Format::PVRTC12A:
        DecompressPVRTC(srcImage, dstImage, 1);
        break;
    case Image::Format::PVRTC14:
    case Image::Format::PVRTC14A:
        DecompressPVRTC(srcImage, dstImage, 0);
        break;
    case Image::Format::ETC1:
        DecompressETC1(srcImage, dstImage);
        break;
    case Image::Format::ETC2:
        DecompressETC2_RGB8(srcImage, dstImage);
        break;
    case Image::Format::ETC2A1:
        DecompressETC2_RGB8A1(srcImage, dstImage);
        break;
    case Image::Format::ETC2A:
        DecompressETC2_RGBA8(srcImage, dstImage);
        break;
    case Image::Format::EACR11:
        DecompressEAC_R11(srcImage, dstImage, false);
        break;
    case Image::Format::EACR11_SNORM:
        DecompressEAC_R11(srcImage, dstImage, true);
        break;
    case Image::Format::EACRG11:
        // Consider RG_11_11_EAC format image as normal values.
        DecompressEAC_RG11(srcImage, dstImage, false, true);
        break;
    case Image::Format::EACRG11_SNORM:
        // Consider SignedRG_11_11_EAC format image as normal values.
        DecompressEAC_RG11(srcImage, dstImage, true, true);
        break;
    default:
        BE_WARNLOG("DecompressImage: unsupported format %s\n", srcImage.FormatName());
        return false;
    }

    return true;
}

static bool CompressImage(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    assert(srcImage.GetPixels());

    //uint64_t startClocks = rdtsc();

    switch (dstImage.GetFormat()) {
    case Image::Format::BC1:
        CompressBC1(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::BC2:
        CompressBC2(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::BC3:
        CompressBC3(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::BC4:
        CompressBC4(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::BC5:
        CompressBC5(srcImage, dstImage, compressionQuality);
        break;
    case Image::Format::ETC1:
        return CompressETC1(srcImage, dstImage, compressionQuality);
    case Image::Format::ETC2:
        return CompressETC2_RGB8(srcImage, dstImage, compressionQuality);
    case Image::Format::ETC2A1:
        return CompressETC2_RGBA1(srcImage, dstImage, compressionQuality);
    case Image::Format::ETC2A:
        return CompressETC2_RGBA8(srcImage, dstImage, compressionQuality);
    case Image::Format::EACR11:
        return CompressEAC_R11(srcImage, dstImage, compressionQuality);
    case Image::Format::EACR11_SNORM:
        return CompressEAC_Signed_R11(srcImage, dstImage, compressionQuality);
    case Image::Format::EACRG11:
        return CompressEAC_RG11(srcImage, dstImage, compressionQuality);
    case Image::Format::EACRG11_SNORM:
        return CompressEAC_Signed_RG11(srcImage, dstImage, compressionQuality);
    default:
        BE_WARNLOG("CompressImage: unsupported format %s\n", dstImage.FormatName());
        return false;
    }

    //uint64_t endClocks = rdtsc();
    //BE_LOG("%BC compression in %" PRIu64 " clocks\n", endClocks - startClocks);

    return true;
}

template <float (*ConversionFunc)(float)>
static void ConvertGammaRGB(float *data, int numPixels) {
    for (int i = 0; i < numPixels; i++, data += 4) {
        data[0] = ConversionFunc(data[0]);
        data[1] = ConversionFunc(data[1]);
        data[2] = ConversionFunc(data[2]);
    }
}

static void SRGBToLinear(float *data, int numPixels) {
    ConvertGammaRGB<Image::GammaToLinearApprox>(data, numPixels);
}

static void Pow22ToLinear(float *data, int numPixels) {
    ConvertGammaRGB<Image::GammaToLinearFast>(data, numPixels);
}

static void LinearToSRGB(float *data, int numPixels) {
    ConvertGammaRGB<Image::LinearToGammaApprox>(data, numPixels);
}

static void LinearToPow22(float *data, int numPixels) {
    ConvertGammaRGB<Image::LinearToGammaFast>(data, numPixels);
}

static float SRGBToPow22Value(float value) {
    return Image::LinearToGammaFast(Image::GammaToLinearApprox(value));
}

static float Pow22ToSRGBValue(float value) {
    return Image::LinearToGammaApprox(Image::GammaToLinearFast(value));
}

static void SRGBToPow22(float *data, int numPixels) {
    ConvertGammaRGB<SRGBToPow22Value>(data, numPixels);
}

static void Pow22ToSRGB(float *data, int numPixels) {
    ConvertGammaRGB<Pow22ToSRGBValue>(data, numPixels);
}

using GammaConversionFunc = void (*)(float *data, int count);

static GammaConversionFunc GetGammaConversionFunc(Image::GammaSpace srcGammaSpace, Image::GammaSpace dstGammaSpace) {
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

bool Image::ConvertFormat(Format dstFormat, Image &dstImage, GammaSpace dstGammaSpace, MipmapGenerationMode mipmapRegenerationMode, CompressionQuality compressionQuality) const {
    if (&dstImage == this) {
        Image tempImage;
        if (!ConvertFormat(dstFormat, tempImage, dstGammaSpace, mipmapRegenerationMode, compressionQuality)) {
            return false;
        }
        dstImage = std::move(tempImage);
        return true;
    }

    if (dstGammaSpace == GammaSpace::DontCare) {
        dstGammaSpace = NeedFloatConversion(dstFormat) ? GammaSpace::Linear : gammaSpace;
    }

    // If the source and destination formats are the same, copy the source image to the destination image and return.
    if (format == dstFormat && gammaSpace == dstGammaSpace && mipmapRegenerationMode == MipmapGenerationMode::NoMipmaps) {
        dstImage = *this;
        return true;
    }

    const Image *srcImage = this;

    // Calculate the mipmap count for the destination image.
    int numDstMipmaps = mipmapRegenerationMode != MipmapGenerationMode::NoMipmaps ? MaxMipLevels(width, height, depth) : numMipLevels;

    // Create a destination image based on the source (this) image.
    dstImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->arraySize, numDstMipmaps, dstFormat, dstGammaSpace, nullptr, srcImage->flags);

    Image unpackedSrcImage;
    Image convertedSrcImage;

    if (srcImage->IsCompressed()) {
        // If the source image is compressed, decompress it first.
        unpackedSrcImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->arraySize, numDstMipmaps, srcImage->NeedFloatConversion() ? Format::R32G32B32A32_FLOAT : Format::R8G8B8A8, srcImage->gammaSpace, nullptr, srcImage->flags);

        if (!DecompressImage(*srcImage, unpackedSrcImage)) {
            dstImage.Clear();
            return false;
        }

        if (mipmapRegenerationMode != MipmapGenerationMode::NoMipmaps) {
            unpackedSrcImage.GenerateMipmaps(mipmapRegenerationMode == MipmapGenerationMode::MipmapsWithAlphaCoverage);
        }

        srcImage = &unpackedSrcImage;
    } else if (mipmapRegenerationMode != MipmapGenerationMode::NoMipmaps) {
        if (!srcImage->IsPacked()) {
            unpackedSrcImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->arraySize, numDstMipmaps, srcImage->format, srcImage->gammaSpace, nullptr, srcImage->flags);

            // Copy only the first level of the source image for mipmap generation.
            unpackedSrcImage.CopyFrom(*srcImage, 0, 1);
        } else {
            // Packed format can't generate mipmaps directly, so we need to unpack it.
            if (!srcImage->ConvertFormat(srcImage->NeedFloatConversion() ? Format::R32G32B32A32_FLOAT : Format::R8G8B8A8, convertedSrcImage, dstGammaSpace)) {
                dstImage.Clear();
                return false;
            }
            unpackedSrcImage.Create(srcImage->width, srcImage->height, srcImage->depth, srcImage->arraySize, numDstMipmaps, convertedSrcImage.format, convertedSrcImage.gammaSpace, nullptr, srcImage->flags);
            unpackedSrcImage.CopyFrom(convertedSrcImage, 0, 1);
        }

        unpackedSrcImage.GenerateMipmaps(mipmapRegenerationMode == MipmapGenerationMode::MipmapsWithAlphaCoverage);

        srcImage = &unpackedSrcImage;
    }

    // If the destination format is compressed, we need to convert source image to R8G8B8A8 or R32G32B32A32_FLOAT first.
    if (Image::IsCompressed(dstFormat)) {
        Image tempImage;

        Format compressorInputFormat = Format::R8G8B8A8;

        switch (dstFormat) {
        case Format::BC1:
        case Format::BC2:
        case Format::BC3:
        case Format::BC4:
        case Format::BC5:
            // BC1~BC5 encoders require R8G8B8A8 input.
            break;
        default:
            if (Image::NeedFloatConversion(dstFormat) || srcImage->gammaSpace != dstGammaSpace) {
                compressorInputFormat = Format::R32G32B32A32_FLOAT;
            }
            break;
        }

        if (srcImage->GetFormat() != compressorInputFormat || srcImage->gammaSpace != dstGammaSpace) {
            if (!srcImage->ConvertFormat(compressorInputFormat, tempImage, dstGammaSpace)) {
                dstImage.Clear();
                return false;
            }
            srcImage = &tempImage;
        }

        if (!CompressImage(*srcImage, dstImage, compressionQuality)) {
            dstImage.Clear();
            return false;
        }
        return true;
    }

    ImageUnpackFunc unpackFunc;
    ImagePackFunc packFunc;
    int bytesPerComponent;

    const ImageFormatInfo *srcFormatInfo = GetImageFormatInfo(srcImage->GetFormat());
    const ImageFormatInfo *dstFormatInfo = GetImageFormatInfo(dstFormat);

    GammaConversionFunc gammaConversionFunc = GetGammaConversionFunc(srcImage->gammaSpace, dstGammaSpace);

    if (gammaConversionFunc || NeedFloatConversion(srcImage->GetFormat()) || NeedFloatConversion(dstFormat)) {
        unpackFunc = srcFormatInfo->unpackRGBA32F;
        packFunc = dstFormatInfo->packRGBA32F;
        bytesPerComponent = sizeof(float);
    } else {
        unpackFunc = srcFormatInfo->unpackRGBA8888;
        packFunc = dstFormatInfo->packRGBA8888;
        bytesPerComponent = sizeof(byte);
    }

    if (!unpackFunc || !packFunc) {
        BE_WARNLOG("Image::ConvertFormat: unsupported convert type (from %s to %s)\n", srcImage->FormatName(), dstImage.FormatName());
        dstImage.Clear();
        return false;
    }

    byte *unpackedBuffer = (byte *)Mem_Alloc16(width * 4 * bytesPerComponent);

    const byte *srcPtr = srcImage->GetPixels();
    byte *dstPtr = dstImage.GetPixels();

    for (int sliceIndex = 0; sliceIndex < srcImage->arraySize; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < srcImage->numMipLevels; mipLevel++) {
            int w = srcImage->GetWidth(mipLevel);
            int h = srcImage->GetHeight(mipLevel);
            int d = srcImage->GetDepth(mipLevel);

            int srcPitch = srcImage->BytesPerPixel() * w;
            int dstPitch = dstImage.BytesPerPixel() * w;

            for (int z = 0; z < d; z++) {
                for (int y = 0; y < h; y++) {
                    unpackFunc(srcPtr, unpackedBuffer, w);

                    if (gammaConversionFunc) {
                        gammaConversionFunc((float *)unpackedBuffer, w);
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

bool Image::ConvertFormatSelf(Format dstFormat, GammaSpace dstGammaSpace, MipmapGenerationMode mipmapRegenerationMode, CompressionQuality compressionQuality) {
    Image dstImage;
    bool ret = ConvertFormat(dstFormat, dstImage, dstGammaSpace, mipmapRegenerationMode, compressionQuality);
    if (ret) {
        *this = std::move(dstImage);
    }
    return ret;
}

BE_NAMESPACE_END
