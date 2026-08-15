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
#include "Core/Heap.h"
#include "Image/Image.h"
#include "ImageInternal.h"
#include "etc2comp/EtcLib/Etc/Etc.h"

BE_NAMESPACE_BEGIN

#define MIN_JOBS 8
#define MAX_JOBS 1024

static float QualityToEffort(Image::CompressionQuality compressionQuality) {
    switch (compressionQuality) {
    case Image::CompressionQuality::HighQuality:
        return 80;
    case Image::CompressionQuality::Normal:
        return 40;
    case Image::CompressionQuality::Fast:
        return 20;
    }
    return 0;
}

static bool EncodeETC(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality, Etc::Image::Format format, Etc::ErrorMetric errorMetric) {
    assert(srcImage.GetFormat() == Image::Format::R8G8B8A8 || srcImage.GetFormat() == Image::Format::R32G32B32A32_FLOAT);

    Etc::ColorFloatRGBA *temp = nullptr;

    if (!srcImage.IsFloatFormat()) {
        temp = (Etc::ColorFloatRGBA *)Mem_Alloc16(srcImage.GetWidth() * srcImage.GetHeight() * sizeof(Etc::ColorFloatRGBA));
    }

    const float effort = QualityToEffort(compressionQuality);
    const int numMipmaps = srcImage.NumMipmaps();
    const int numSlices = srcImage.GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            const int w = srcImage.GetWidth(mipLevel);
            const int h = srcImage.GetHeight(mipLevel);
            const int d = srcImage.GetDepth(mipLevel);
            const size_t srcPlaneSize = static_cast<size_t>(w) * h * srcImage.BytesPerPixel();
            const size_t dstPlaneSize = dstImage.SizeInBytesForSlice(mipLevel, 1) / d;

            for (int z = 0; z < d; z++) {
                byte *src = srcImage.GetPixels(mipLevel, sliceIndex) + z * srcPlaneSize;
                byte *dst = dstImage.GetPixels(mipLevel, sliceIndex) + z * dstPlaneSize;

                Etc::ColorFloatRGBA *fsrc;

                if (temp) {
                    // Convert byte RGBA_8_8_8_8 to float RGBA.
                    Etc::ColorFloatRGBA *fsrcPtr = temp;
                    for (byte *srcEnd = src + srcPlaneSize; src < srcEnd; src += 4) {
                        *fsrcPtr++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(src[0], src[1], src[2], src[3]);
                    }
                    fsrc = temp;
                } else {
                    fsrc = (Etc::ColorFloatRGBA *)src;
                }

                // Encode.
                Etc::Image image((float *)fsrc, w, h, errorMetric);
                image.m_bVerboseOutput = false;
                const Etc::Image::EncodingStatus status = image.Encode(format, errorMetric, effort, MIN_JOBS, MAX_JOBS);
                if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD) {
                    if (temp) {
                        Mem_AlignedFree(temp);
                    }
                    return false;
                }

                // Write to destination memory.
                const size_t encodedBytes = image.GetEncodingBitsBytes();
                assert(encodedBytes == dstPlaneSize);
                memcpy(dst, image.GetEncodingBits(), encodedBytes);
            }
        }
    }

    if (temp) {
        Mem_AlignedFree(temp);
    }
    return true;
}

bool CompressETC1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::ETC1, Etc::ErrorMetric::RGBX);
}

bool CompressETC2_RGB8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::RGB8, Etc::ErrorMetric::RGBX);
}

bool CompressETC2_RGBA1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::RGB8A1, Etc::ErrorMetric::RGBA);
}

bool CompressETC2_RGBA8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::RGBA8, Etc::ErrorMetric::RGBA);
}

bool CompressEAC_R11(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::R11, Etc::ErrorMetric::NUMERIC);
}

bool CompressEAC_RG11(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::RG11, Etc::ErrorMetric::NORMALXYZ);
}

bool CompressEAC_Signed_R11(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::SIGNED_R11, Etc::ErrorMetric::NUMERIC);
}

bool CompressEAC_Signed_RG11(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressionQuality) {
    return EncodeETC(srcImage, dstImage, compressionQuality, Etc::Image::Format::SIGNED_RG11, Etc::ErrorMetric::NORMALXYZ);
}

BE_NAMESPACE_END
