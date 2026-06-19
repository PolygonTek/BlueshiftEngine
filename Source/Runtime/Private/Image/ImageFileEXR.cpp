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
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"
#include "zlib/zlib.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

BE_NAMESPACE_BEGIN

struct EXRScopedData {
    EXRScopedData() {
        InitEXRHeader(&header);
        InitEXRImage(&image);
    }

    ~EXRScopedData() {
        FreeEXRImage(&image);
        FreeEXRHeader(&header);
    }

    EXRHeader header;
    EXRImage image;
};

static void WarnEXRError(const char *name, const char *message, const char *err) {
    if (err) {
        BE_WARNLOG("Image::LoadEXRFromMemory: %s %s (%s)\n", message, name, err);
        FreeEXRErrorMessage(err);
    } else {
        BE_WARNLOG("Image::LoadEXRFromMemory: %s %s\n", message, name);
    }
}

static bool IsExactChannelName(const char *name, const char *channelName) {
    return !strcmp(name, channelName);
}

static bool IsLayeredChannelName(const char *name, const char *channelName) {
    const size_t nameLength = strlen(name);
    const size_t channelNameLength = strlen(channelName);

    return nameLength > channelNameLength &&
        name[nameLength - channelNameLength - 1] == '.' &&
        !strcmp(name + nameLength - channelNameLength, channelName);
}

static int FindEXRChannel(const EXRHeader &header, const char *channelName) {
    for (int i = 0; i < header.num_channels; i++) {
        if (IsExactChannelName(header.channels[i].name, channelName)) {
            return i;
        }
    }

    for (int i = 0; i < header.num_channels; i++) {
        if (IsLayeredChannelName(header.channels[i].name, channelName)) {
            return i;
        }
    }

    return -1;
}

static bool IsSupportedEXRPixelType(int pixelType) {
    return pixelType == TINYEXR_PIXELTYPE_HALF ||
        pixelType == TINYEXR_PIXELTYPE_FLOAT ||
        pixelType == TINYEXR_PIXELTYPE_UINT;
}

static bool NeedsFloat32Output(const EXRHeader &header, int redIndex, int greenIndex, int blueIndex, int alphaIndex, bool hasAlpha) {
    return header.pixel_types[redIndex] != TINYEXR_PIXELTYPE_HALF ||
        header.pixel_types[greenIndex] != TINYEXR_PIXELTYPE_HALF ||
        header.pixel_types[blueIndex] != TINYEXR_PIXELTYPE_HALF ||
        (hasAlpha && header.pixel_types[alphaIndex] != TINYEXR_PIXELTYPE_HALF);
}

static float ReadEXRChannelAsFloat(const EXRHeader &header, unsigned char **src, int channelIndex, int pixelIndex) {
    switch (header.pixel_types[channelIndex]) {
    case TINYEXR_PIXELTYPE_HALF:
        return F16Converter::ToF32(reinterpret_cast<const float16_t *>(src[channelIndex])[pixelIndex]);
    case TINYEXR_PIXELTYPE_FLOAT:
        return reinterpret_cast<const float *>(src[channelIndex])[pixelIndex];
    case TINYEXR_PIXELTYPE_UINT:
        return static_cast<float>(reinterpret_cast<const uint32_t *>(src[channelIndex])[pixelIndex]);
    default:
        return 0.0f;
    }
}

static float16_t ReadEXRChannelAsHalf(const EXRHeader &header, unsigned char **src, int channelIndex, int pixelIndex) {
    if (header.pixel_types[channelIndex] == TINYEXR_PIXELTYPE_HALF) {
        return reinterpret_cast<const float16_t *>(src[channelIndex])[pixelIndex];
    }

    return F16Converter::FromF32(ReadEXRChannelAsFloat(header, src, channelIndex, pixelIndex));
}

static void WriteEXRPixelAsHalf(float16_t *dst, const EXRHeader &header, unsigned char **src, int pixelIndex, int redIndex, int greenIndex, int blueIndex, int alphaIndex, bool hasAlpha) {
    dst[0] = ReadEXRChannelAsHalf(header, src, redIndex, pixelIndex);
    dst[1] = ReadEXRChannelAsHalf(header, src, greenIndex, pixelIndex);
    dst[2] = ReadEXRChannelAsHalf(header, src, blueIndex, pixelIndex);

    if (hasAlpha) {
        dst[3] = alphaIndex >= 0 ? ReadEXRChannelAsHalf(header, src, alphaIndex, pixelIndex) : F16Converter::FromF32(1.0f);
    }
}

static void WriteEXRPixelAsFloat(float *dst, const EXRHeader &header, unsigned char **src, int pixelIndex, int redIndex, int greenIndex, int blueIndex, int alphaIndex, bool hasAlpha) {
    dst[0] = ReadEXRChannelAsFloat(header, src, redIndex, pixelIndex);
    dst[1] = ReadEXRChannelAsFloat(header, src, greenIndex, pixelIndex);
    dst[2] = ReadEXRChannelAsFloat(header, src, blueIndex, pixelIndex);

    if (hasAlpha) {
        dst[3] = alphaIndex >= 0 ? ReadEXRChannelAsFloat(header, src, alphaIndex, pixelIndex) : 1.0f;
    }
}

static bool CopyScanlineEXRImage(byte *dst, bool useFloat32, const EXRHeader &header, const EXRImage &image, int redIndex, int greenIndex, int blueIndex, int alphaIndex, bool hasAlpha) {
    if (!image.images) {
        return false;
    }

    const int numComponents = hasAlpha ? 4 : 3;
    const size_t pixelCount = static_cast<size_t>(image.width) * image.height;

    if (useFloat32) {
        float *dstFloat = reinterpret_cast<float *>(dst);
        for (size_t i = 0; i < pixelCount; i++) {
            WriteEXRPixelAsFloat(dstFloat + i * numComponents, header, image.images, static_cast<int>(i), redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);
        }
    } else {
        float16_t *dstHalf = reinterpret_cast<float16_t *>(dst);
        for (size_t i = 0; i < pixelCount; i++) {
            WriteEXRPixelAsHalf(dstHalf + i * numComponents, header, image.images, static_cast<int>(i), redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);
        }
    }

    return true;
}

static bool CopyTiledEXRImage(byte *dst, bool useFloat32, const EXRHeader &header, const EXRImage &image, int redIndex, int greenIndex, int blueIndex, int alphaIndex, bool hasAlpha) {
    if (!image.tiles || header.tile_size_x <= 0 || header.tile_size_y <= 0) {
        return false;
    }

    const int numComponents = hasAlpha ? 4 : 3;
    float *dstFloat = reinterpret_cast<float *>(dst);
    float16_t *dstHalf = reinterpret_cast<float16_t *>(dst);

    for (int tileIndex = 0; tileIndex < image.num_tiles; tileIndex++) {
        const EXRTile &tile = image.tiles[tileIndex];
        if (!tile.images) {
            return false;
        }

        for (int y = 0; y < tile.height; y++) {
            const int dstY = tile.offset_y * header.tile_size_y + y;
            if (dstY < 0 || dstY >= image.height) {
                continue;
            }

            for (int x = 0; x < tile.width; x++) {
                const int dstX = tile.offset_x * header.tile_size_x + x;
                if (dstX < 0 || dstX >= image.width) {
                    continue;
                }

                const int srcIndex = y * header.tile_size_x + x;
                const size_t dstIndex = (static_cast<size_t>(dstY) * image.width + dstX) * numComponents;
                if (useFloat32) {
                    WriteEXRPixelAsFloat(dstFloat + dstIndex, header, tile.images, srcIndex, redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);
                } else {
                    WriteEXRPixelAsHalf(dstHalf + dstIndex, header, tile.images, srcIndex, redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);
                }
            }
        }
    }

    return true;
}

bool Image::LoadEXRFromMemory(const char *name, const byte *data, size_t size) {
    if (!data || !size) {
        return false;
    }

    const char *err = nullptr;
    EXRVersion version;
    int ret = ParseEXRVersionFromMemory(&version, data, size);
    if (ret != TINYEXR_SUCCESS) {
        WarnEXRError(name, "bad EXR version", err);
        return false;
    }

    if (version.multipart || version.non_image) {
        BE_WARNLOG("Image::LoadEXRFromMemory: unsupported multipart/deep EXR %s\n", name);
        return false;
    }

    EXRScopedData exr;
    ret = ParseEXRHeaderFromMemory(&exr.header, &version, data, size, &err);
    if (ret != TINYEXR_SUCCESS) {
        WarnEXRError(name, "bad EXR header", err);
        return false;
    }

    int redIndex = FindEXRChannel(exr.header, "R");
    int greenIndex = FindEXRChannel(exr.header, "G");
    int blueIndex = FindEXRChannel(exr.header, "B");
    const int alphaIndex = FindEXRChannel(exr.header, "A");

    if (redIndex < 0 || greenIndex < 0 || blueIndex < 0) {
        if (exr.header.num_channels != 1) {
            BE_WARNLOG("Image::LoadEXRFromMemory: RGB channels not found %s\n", name);
            return false;
        }

        redIndex = greenIndex = blueIndex = 0;
    }

    if (!IsSupportedEXRPixelType(exr.header.pixel_types[redIndex]) ||
        !IsSupportedEXRPixelType(exr.header.pixel_types[greenIndex]) ||
        !IsSupportedEXRPixelType(exr.header.pixel_types[blueIndex]) ||
        (alphaIndex >= 0 && !IsSupportedEXRPixelType(exr.header.pixel_types[alphaIndex]))) {
        BE_WARNLOG("Image::LoadEXRFromMemory: unsupported pixel type %s\n", name);
        return false;
    }

    ret = LoadEXRImageFromMemory(&exr.image, &exr.header, data, size, &err);
    if (ret != TINYEXR_SUCCESS) {
        WarnEXRError(name, "failed to load EXR image", err);
        return false;
    }

    if (exr.image.width <= 0 || exr.image.height <= 0) {
        BE_WARNLOG("Image::LoadEXRFromMemory: bad image size %s\n", name);
        return false;
    }

    const bool hasAlpha = alphaIndex >= 0;
    const bool useFloat32 = NeedsFloat32Output(exr.header, redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);
    const Format dstFormat = useFloat32 ?
        (hasAlpha ? Format::R32G32B32A32_FLOAT : Format::R32G32B32_FLOAT) :
        (hasAlpha ? Format::R16G16B16A16_FLOAT : Format::R16G16B16_FLOAT);
    const uint64_t dstSize = MemRequired(exr.image.width, exr.image.height, 1, 1, 1, dstFormat);
    if (dstSize > INT_MAX) {
        BE_WARNLOG("Image::LoadEXRFromMemory: image is too large %s\n", name);
        return false;
    }

    Create2D(exr.image.width, exr.image.height, 1, dstFormat, GammaSpace::Linear, nullptr, Flag::None);

    const bool copied = exr.header.tiled ?
        CopyTiledEXRImage(pic, useFloat32, exr.header, exr.image, redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha) :
        CopyScanlineEXRImage(pic, useFloat32, exr.header, exr.image, redIndex, greenIndex, blueIndex, alphaIndex, hasAlpha);

    if (!copied) {
        Clear();
        BE_WARNLOG("Image::LoadEXRFromMemory: failed to copy EXR pixels %s\n", name);
        return false;
    }

    return true;
}

BE_NAMESPACE_END
