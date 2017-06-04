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
#include "Core/Heap.h"
#include "Image/Image.h"
#include "ImageInternal.h"
#include "etc2comp/EtcLib/Etc/Etc.h"

BE_NAMESPACE_BEGIN

#define MAX_JOBS 1024

static float QualityToEffort(Image::CompressionQuality compressoinQuality) {   
    switch (compressoinQuality) {
    case Image::HighQuality:
        return 2; // speed is important than quality
    case Image::Normal:
        return 1;
    case Image::Fast:
        return 0;
    }
    return 0;
}

void CompressETC1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    Etc::ColorFloatRGBA *fsrc = (Etc::ColorFloatRGBA *)Mem_Alloc16(srcImage.GetWidth() * srcImage.GetHeight() * sizeof(Etc::ColorFloatRGBA));

    float effort = QualityToEffort(compressoinQuality);

    int numMipmaps = srcImage.NumMipmaps();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);

        byte *src = srcImage.GetPixels(mipLevel);
        byte *dst = dstImage.GetPixels(mipLevel);

        // Convert byte RGBA_8_8_8_8 to float RGBA
        Etc::ColorFloatRGBA *fsrc_ptr = fsrc;
        for (byte *src_end = &src[w * h * 4]; src < src_end; src += 4) {
            *fsrc_ptr++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(src[0], src[1], src[2], src[3]);
        }

        // Encode ETC1 RGB
        Etc::Image image((float *)fsrc, w, h, Etc::ErrorMetric::REC709);
        image.m_bVerboseOutput = false;
        Etc::Image::EncodingStatus status = image.Encode(Etc::Image::Format::ETC1, Etc::ErrorMetric::REC709, effort, 8, MAX_JOBS);
        if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD) {
            assert(0);
            return;
        }

        // Write to destination memory
        size_t encodedBytes = image.GetEncodingBitsBytes();
        assert(encodedBytes == dstImage.GetSize(mipLevel));
        memcpy(dst, image.GetEncodingBits(), encodedBytes);
    }

    Mem_AlignedFree(fsrc);
}

void CompressETC2_RGB8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    Etc::ColorFloatRGBA *fsrc = (Etc::ColorFloatRGBA *)Mem_Alloc16(srcImage.GetWidth() * srcImage.GetHeight() * sizeof(Etc::ColorFloatRGBA));
    
    float effort = QualityToEffort(compressoinQuality);

    int numMipmaps = srcImage.NumMipmaps();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);

        byte *src = srcImage.GetPixels(mipLevel);
        byte *dst = dstImage.GetPixels(mipLevel);

        // Convert byte RGBA_8_8_8_8 to float RGBA
        Etc::ColorFloatRGBA *fsrc_ptr = fsrc;
        for (byte *src_end = &src[w * h * 4]; src < src_end; src += 4) {
            *fsrc_ptr++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(src[0], src[1], src[2], src[3]);
        }

        // Encode ETC2 RGB8
        Etc::Image image((float *)fsrc, w, h, Etc::ErrorMetric::REC709);
        image.m_bVerboseOutput = false;
        Etc::Image::EncodingStatus status = image.Encode(Etc::Image::Format::RGB8, Etc::ErrorMetric::REC709, effort, 8, MAX_JOBS);
        if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD) {
            assert(0);
            return;
        }

        // Write to destination memory
        size_t encodedBytes = image.GetEncodingBitsBytes();
        assert(encodedBytes == dstImage.GetSize(mipLevel));
        memcpy(dst, image.GetEncodingBits(), encodedBytes);
    }

    Mem_AlignedFree(fsrc);
}

void CompressETC2_RGBA8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    Etc::ColorFloatRGBA *fsrc = (Etc::ColorFloatRGBA *)Mem_Alloc16(srcImage.GetWidth() * srcImage.GetHeight() * sizeof(Etc::ColorFloatRGBA));

    float effort = QualityToEffort(compressoinQuality);

    int numMipmaps = srcImage.NumMipmaps();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);

        byte *src = srcImage.GetPixels(mipLevel);
        byte *dst = dstImage.GetPixels(mipLevel);

        // Convert byte RGBA_8_8_8_8 to float RGBA
        Etc::ColorFloatRGBA *fsrc_ptr = fsrc;
        for (byte *src_end = &src[w * h * 4]; src < src_end; src += 4) {
            *fsrc_ptr++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(src[0], src[1], src[2], src[3]);
        }

        // Encode ETC2 RGBA8
        Etc::Image image((float *)fsrc, w, h, Etc::ErrorMetric::REC709);
        image.m_bVerboseOutput = false;
        Etc::Image::EncodingStatus status = image.Encode(Etc::Image::Format::RGBA8, Etc::ErrorMetric::REC709, effort, 8, MAX_JOBS);
        if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD) {
            assert(0);
            return;
        }

        // Write to destination memory
        size_t encodedBytes = image.GetEncodingBitsBytes();
        assert(encodedBytes == dstImage.GetSize(mipLevel));
        memcpy(dst, image.GetEncodingBits(), encodedBytes);
    }

    Mem_AlignedFree(fsrc);
}

void CompressETC2_RGBA1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality) {
    Etc::ColorFloatRGBA *fsrc = (Etc::ColorFloatRGBA *)Mem_Alloc16(srcImage.GetWidth() * srcImage.GetHeight() * sizeof(Etc::ColorFloatRGBA));

    float effort = QualityToEffort(compressoinQuality);

    int numMipmaps = srcImage.NumMipmaps();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);

        byte *src = srcImage.GetPixels(mipLevel);
        byte *dst = dstImage.GetPixels(mipLevel);

        // Convert byte RGBA_8_8_8_8 to float RGBA
        Etc::ColorFloatRGBA *fsrc_ptr = fsrc;
        for (byte *src_end = &src[w * h * 4]; src < src_end; src += 4) {
            *fsrc_ptr++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(src[0], src[1], src[2], src[3]);
        }

        // Encode ETC2 RGB8A1
        Etc::Image image((float *)fsrc, w, h, Etc::ErrorMetric::REC709);
        image.m_bVerboseOutput = false;
        Etc::Image::EncodingStatus status = image.Encode(Etc::Image::Format::RGB8A1, Etc::ErrorMetric::REC709, effort, 8, MAX_JOBS);
        if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD) {
            assert(0);
            return;
        }

        // Write to destination memory
        size_t encodedBytes = image.GetEncodingBitsBytes();
        assert(encodedBytes == dstImage.GetSize(mipLevel));
        memcpy(dst, image.GetEncodingBits(), encodedBytes);
    }

    Mem_AlignedFree(fsrc);
}

BE_NAMESPACE_END
