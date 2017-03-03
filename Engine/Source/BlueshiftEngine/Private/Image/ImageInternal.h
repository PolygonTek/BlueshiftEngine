#pragma once

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
// various pack/unpack function type for each color format
//--------------------------------------------------------------------------------------------------
using UserFormatToRGBA8888Func = void (*)(const byte *src, byte *dst, int numPixels);
using RGBA8888ToUserFormatFunc = void (*)(const byte *src, byte *dst, int numPixels);

struct ImageFormatInfo {
    const char *name;
    int size; // bytes per pixel or bytes per block
    int numComponents;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int type;
    UserFormatToRGBA8888Func unpackFunc;
    RGBA8888ToUserFormatFunc packFunc;
};

void DecompressPVRTC(const Image &srcImage, Image &dstImage, int do2BitMode);

void DecompressETC(const Image &srcImage, Image &dstImage, int nMode);

bool CompressedFormatBlockDimensions(Image::Format imageFormat, int &blockWidth, int &blockHeight);
bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight);

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat);

void RGBToYCoCg(short *YCoCg, const byte *rgb, int stride);
void RGBAToYCoCgA(short *YCoCgA, const byte *rgba, int stride);
void YCoCgToRGB(byte *rgb, int stride, const short *YCoCg);
void YCoCgAToRGBA(byte *rgba, int stride, const short *YCoCgA);

BE_NAMESPACE_END
