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
#include "SIMD/Simd.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

Image &Image::InitFromMemory(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, byte *data, int flags) {
    Clear();

    this->width = width;
    this->height = height;
    this->depth = depth;
    this->numSlices = Max(numSlices, 1);
    this->numMipmaps = Max(numMipmaps, 1);
    this->format = format;
    this->flags = flags;
    this->allocated = false;
    this->pic = data;

    return *this;
}

Image &Image::Create(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, const byte *data, int flags) {
    Clear();

    this->width = width;
    this->height = height;
    this->depth = depth;
    this->numSlices = Max(numSlices, 1);
    this->numMipmaps = Max(numMipmaps, 1);
    this->format = format;
    this->flags = flags;
    
    int size = GetSize(0, numMipmaps);	
    this->pic = (byte *)Mem_Alloc16(size);
    this->allocated = true;
    
    if (data) {
        simdProcessor->Memcpy(this->pic, data, size);
    }

    return *this;
}

Image &Image::CreateCubeFromMultipleImages(const Image *images) {	
    Clear();

    this->width = images[0].width;
    this->height = this->width;
    this->depth = 1;
    this->numSlices = 6;
    this->numMipmaps = images[0].numMipmaps;
    this->format = images[0].format;
    this->flags = images[0].flags;
    
    int sliceSize = GetSliceSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(sliceSize * 6);
    this->allocated = true;
    
    byte *dst = this->pic;
    
    for (int i = 0; i < 6; i++) {
        assert(images[i].width == images[i].height);
        assert(images[i].width == images[0].width);
        assert(images[i].format == images[0].format);
        assert(images[i].numMipmaps == images[0].numMipmaps);

        simdProcessor->Memcpy(dst, images[i].pic, sliceSize);
        dst += sliceSize;
    }

    return *this;
}

Image &Image::CopyFrom(const Image &srcImage, int firstLevel, int numLevels) {
    if (srcImage.GetWidth() != width || srcImage.GetHeight() != height || srcImage.GetDepth() != depth || srcImage.NumSlices() != numSlices) {
        return *this;
    }

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        byte *src = srcImage.GetPixels(firstLevel, sliceIndex);
        byte *dst = GetPixels(firstLevel, sliceIndex);
        int size = srcImage.GetSliceSize(firstLevel, numLevels);

        simdProcessor->Memcpy(dst, src, size);
    }

    return *this;
}

Image &Image::operator=(const Image &rhs) {
    Clear();
    Create(rhs.width, rhs.height, rhs.depth, rhs.numSlices, rhs.numMipmaps, rhs.format, rhs.pic, rhs.flags);
    return (*this);
}

Image &Image::operator=(Image &&rhs) {
    Clear();

    width = rhs.width;
    height = rhs.height;
    depth = rhs.depth;
    numSlices = rhs.numSlices;
    numMipmaps = rhs.numMipmaps;
    format = rhs.format;
    flags = rhs.flags;
    allocated = rhs.allocated;
    pic = rhs.pic;

    rhs.allocated = false;
    rhs.pic = nullptr;
    
    return (*this);
}

void Image::Clear() {
    if (allocated && pic) {
        Mem_AlignedFree(pic);
        pic = nullptr;
        allocated = false;
    }
}

Color4 Image::GetColor(int x, int y) const {
    const ImageFormatInfo *info = GetImageFormatInfo(format);
    // FIXME: Fix for compressed format
    int bpp = BytesPerPixel();
    int offset = ((height - 1 - y) * width + x) * bpp;

    ALIGN16(Color4 color);

    if (info->type & (Float | Half)) {
        if (info->type & Half) {
            const float16_t *hsrc = (const float16_t *)&pic[offset];
            color.r = F16toF32(hsrc[0]);
            color.g = F16toF32(hsrc[1]);
            color.b = F16toF32(hsrc[2]);
            color.a = 1.0f;
        } else {
            const float *fsrc = (const float *)&pic[offset];
            color.r = fsrc[0];
            color.g = fsrc[1];
            color.b = fsrc[2];
            color.a = 1.0f;
        }
    } else {
        byte rgba8888[4];
        info->unpackFunc(&pic[offset], rgba8888, 1);
        color.r = rgba8888[0] / 255.0f;
        color.g = rgba8888[1] / 255.0f;
        color.b = rgba8888[2] / 255.0f;
        color.a = rgba8888[3] / 255.0f;
    }

    return color;
}

int Image::NumPixels(int firstLevel, int numLevels) const {
    int w = GetWidth(firstLevel);
    int h = GetHeight(firstLevel);
    int d = GetDepth(firstLevel);
    
    int size = 0;
    while (numLevels){
        size += w * h * d;
        w >>= 1;
        h >>= 1;
        d >>= 1;

        if (w == 0 && h == 0 && d == 0) break;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        if (d == 0) d = 1;

        numLevels--;
    }

    return size * numSlices;
}

int Image::GetSize(int firstLevel, int numLevels) const {
    int size = MemRequired(GetWidth(firstLevel), GetHeight(firstLevel), GetDepth(firstLevel), numLevels, format);
    return size * numSlices;
}

int Image::GetSliceSize(int firstLevel, int numLevels) const {
    int size = MemRequired(GetWidth(firstLevel), GetHeight(firstLevel), GetDepth(firstLevel), numLevels, format);
    return size;
}

//--------------------------------------------------------------------------------------------------
//
// static helper functions for image information
//
//--------------------------------------------------------------------------------------------------

const char *Image::FormatName(Image::Format imageFormat) {
    return GetImageFormatInfo(imageFormat)->name;
}

int Image::BytesPerPixel(Image::Format imageFormat) {
    return !IsCompressed(imageFormat) ? GetImageFormatInfo(imageFormat)->size : 0;
}

int Image::BytesPerBlock(Image::Format imageFormat) {
    return IsCompressed(imageFormat) ? GetImageFormatInfo(imageFormat)->size : 0;
}

int Image::NumComponents(Image::Format imageFormat) {
    return GetImageFormatInfo(imageFormat)->numComponents;
}

void Image::GetBits(Image::Format imageFormat, int *redBits, int *greenBits, int *blueBits, int *alphaBits) {
    const ImageFormatInfo *info = GetImageFormatInfo(imageFormat);
    if (redBits)	*redBits	= info->redBits;
    if (greenBits)	*greenBits	= info->greenBits;
    if (blueBits)	*blueBits	= info->blueBits;
    if (alphaBits)	*alphaBits	= info->alphaBits;
}

bool Image::HasAlpha(Image::Format imageFormat) {
    const ImageFormatInfo *info = GetImageFormatInfo(imageFormat);
    return info->alphaBits > 0 ? true : false;
}

bool Image::IsPacked(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Packed);
}

bool Image::IsCompressed(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Compressed);
}

bool Image::IsFloatFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Float);
}

bool Image::IsHalfFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Half);
}

bool Image::IsDepthFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Depth);
}

bool Image::IsDepthStencilFormat(Image::Format imageFormat) {
    return (GetImageFormatInfo(imageFormat)->type & DepthStencil) == DepthStencil;
}

int Image::MemRequired(int width, int height, int depth, int numMipmaps, Image::Format imageFormat) {
    int w = width;
    int h = height;
    int d = depth;

    int size = 0;
    while (numMipmaps) {
        if (IsCompressed(imageFormat)) {
            int blockWidth, blockHeight;
            int minWidth, minHeight;
            CompressedFormatBlockDimensions(imageFormat, blockWidth, blockHeight);
            CompressedFormatMinDimensions(imageFormat, minWidth, minHeight);
            int w2 = w + ((unsigned int)(-w) % minWidth);
            int h2 = h + ((unsigned int)(-h) % minHeight);
            size += (w2 * h2 * d) / (blockWidth * blockHeight);
        } else {
            size += w * h * d;
        }
        w >>= 1;
        h >>= 1;
        d >>= 1;

        if (w == 0 && h == 0 && d == 0) break;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        if (d == 0) d = 1;

        numMipmaps--;
    }

    if (IsCompressed(imageFormat)) {
        size *= BytesPerBlock(imageFormat);
    } else {
        size *= BytesPerPixel(imageFormat);
    }

    return size;
}

int Image::MaxMipMapLevels(int width, int height, int depth) {
    int m = Max3(width, height, depth);

    int numMipLevels = 0;
    while (m > 0) {
        m >>= 1;
        numMipLevels++;
    }

    return numMipLevels;
}

BE_NAMESPACE_END
