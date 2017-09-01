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

#pragma once

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
// various pack/unpack function type for each color format
//--------------------------------------------------------------------------------------------------
using ImageUnpackFunc = void (*)(const byte *src, byte *dst, int numPixels);
using ImagePackFunc = void (*)(const byte *src, byte *dst, int numPixels);

struct ImageFormatInfo {
    const char *name;
    int size; // bytes per pixel or bytes per block
    int numComponents;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int type;
    ImageUnpackFunc unpackRGBA8888;
    ImagePackFunc packRGBA8888;
    ImageUnpackFunc unpackRGBA32F;
    ImagePackFunc packRGBA32F;
};

void DecompressDXT1(const Image &srcImage, Image &dstImage);
void DecompressDXT3(const Image &srcImage, Image &dstImage);
void DecompressDXT5(const Image &srcImage, Image &dstImage);
void DecompressDXN2(const Image &srcImage, Image &dstImage);

void DecompressPVRTC(const Image &srcImage, Image &dstImage, int do2BitMode);

void DecompressETC1(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGB8(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGBA8(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGB8A1(const Image &srcImage, Image &dstImage);

void CompressDXT1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressDXT3(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressDXT5(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressDXN2(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);

void CompressETC1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressETC2_RGB8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressETC2_RGBA8(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);
void CompressETC2_RGBA1(const Image &srcImage, Image &dstImage, Image::CompressionQuality compressoinQuality);

bool CompressedFormatBlockDimensions(Image::Format imageFormat, int &blockWidth, int &blockHeight);
bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight);

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat);

void RGBToYCoCg(short *YCoCg, const byte *rgb, int stride);
void RGBAToYCoCgA(short *YCoCgA, const byte *rgba, int stride);
void YCoCgToRGB(byte *rgb, int stride, const short *YCoCg);
void YCoCgAToRGBA(byte *rgba, int stride, const short *YCoCgA);

BE_NAMESPACE_END
