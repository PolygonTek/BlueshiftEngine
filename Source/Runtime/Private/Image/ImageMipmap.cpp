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

#include <cfloat>

BE_NAMESPACE_BEGIN

#ifndef IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN
#define IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN 1
#endif

template <typename T>
static BE_INLINE T MipMapSample(T value) {
    return value;
}

// IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN 이 켜져있으면, float/half 이미지의 밉맵 생성 시 INF/NAN 을 clamp 한다.
#if IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN
static BE_INLINE float MipMapSample(float value) {
    if (Math::IsFinite(value)) {
        return value;
    }

    // INF should not propagate through the mip chain.
    if (Math::IsInf(value)) {
        return value < 0.0f ? -FLT_MAX : FLT_MAX;
    }

    return 0.0f; // NaN
}

static BE_INLINE half MipMapSample(half value) {
    if (value.IsFinite()) {
        return value;
    }

    if (value.IsInfinity()) {
        return half(value.IsNegative() ? -(float)HALF_MAX : (float)HALF_MAX);
    }

    return half(0.0f); // NaN
}
#endif

template <typename T>
static BE_INLINE T MipMapStore(double value) {
    return static_cast<T>(value);
}

template <>
BE_INLINE half MipMapStore<half>(double value) {
    return half(static_cast<float>(value));
}

template <typename T>
struct MipMapAccumulator {
    typedef T Type;
};

template <>
struct MipMapAccumulator<half> {
    typedef float Type;
};

template <>
struct MipMapAccumulator<float> {
    typedef double Type;
};

template <>
struct MipMapAccumulator<uint32_t> {
    typedef uint64_t Type;
};

template <>
struct MipMapAccumulator<int32_t> {
    typedef int64_t Type;
};

template <typename T>
struct MipMapCoverageAccumulator {
    typedef float Type;
};

template <>
struct MipMapCoverageAccumulator<float> {
    typedef double Type;
};

template <typename T>
static void BuildMipMap1D(T *dst, const T *src, int width, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    typedef typename MipMapAccumulator<T>::Type Accumulator;
    typedef typename MipMapCoverageAccumulator<T>::Type CoverageAccumulator;

    const int dstWidth = Max(width >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    CoverageAccumulator a0 = 0;
    CoverageAccumulator a1 = 0;
    CoverageAccumulator alphaSum = 0;

    for (int x = 0; x < dstWidth; x++) {
        if (alphaComponentIndex >= 0) {
            a0 = MipMapSample(src[alphaComponentIndex]);
            a1 = MipMapSample(src[alphaComponentIndex + xOff]);
            alphaSum = a0 + a1;
        }

        for (int i = 0; i < numComponents; i++) {
            if (preserveCoverage && alphaSum > 0) {
                if (i != alphaComponentIndex) {
                    CoverageAccumulator p0 = MipMapSample(src[0]) * a0;
                    CoverageAccumulator p1 = MipMapSample(src[xOff]) * a1;
                    *dst++ = (p0 + p1) / alphaSum;
                } else {
                    *dst++ = Max(a0, a1);
                }
            } else {
                Accumulator p0 = MipMapSample(src[0]);
                Accumulator p1 = MipMapSample(src[xOff]);
                *dst++ = MipMapStore<T>((p0 + p1) / static_cast<Accumulator>(2));
            }
            src++;
        }
        src += xOff;
    }
}

template <typename T>
static void BuildMipMap2D(T *dst, const T *src, int width, int height, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    typedef typename MipMapAccumulator<T>::Type Accumulator;
    typedef typename MipMapCoverageAccumulator<T>::Type CoverageAccumulator;

    const int dstWidth = Max(width >> 1, 1);
    const int dstHeight = Max(height >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    const size_t yOff = height < 2 ? 0 : static_cast<size_t>(numComponents) * width;
    const size_t xRemainder = width > 1 && (width & 1) ? numComponents : 0;
    CoverageAccumulator a0 = 0;
    CoverageAccumulator a1 = 0;
    CoverageAccumulator a2 = 0;
    CoverageAccumulator a3 = 0;
    CoverageAccumulator alphaSum = 0;

    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            if (alphaComponentIndex >= 0) {
                a0 = MipMapSample(src[alphaComponentIndex]);
                a1 = MipMapSample(src[alphaComponentIndex + xOff]);
                a2 = MipMapSample(src[alphaComponentIndex + yOff]);
                a3 = MipMapSample(src[alphaComponentIndex + yOff + xOff]);
                alphaSum = a0 + a1 + a2 + a3;
            }

            for (int i = 0; i < numComponents; i++) {
                if (preserveCoverage && alphaSum > 0) {
                    if (i != alphaComponentIndex) {
                        CoverageAccumulator p0 = MipMapSample(src[0]) * a0;
                        CoverageAccumulator p1 = MipMapSample(src[xOff]) * a1;
                        CoverageAccumulator p2 = MipMapSample(src[yOff]) * a2;
                        CoverageAccumulator p3 = MipMapSample(src[yOff + xOff]) * a3;
                        *dst++ = (p0 + p1 + p2 + p3) / alphaSum;
                    } else {
                        *dst++ = Max4(a0, a1, a2, a3);
                    }
                } else {
                    Accumulator p0 = MipMapSample(src[0]);
                    Accumulator p1 = MipMapSample(src[xOff]);
                    Accumulator p2 = MipMapSample(src[yOff]);
                    Accumulator p3 = MipMapSample(src[yOff + xOff]);
                    *dst++ = MipMapStore<T>((p0 + p1 + p2 + p3) / static_cast<Accumulator>(4));
                }
                src++;
            }
            src += xOff;
        }
        src += xRemainder + yOff;
    }
}

template <typename T>
static void BuildMipMap3D(T *dst, const T *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    typedef typename MipMapAccumulator<T>::Type Accumulator;
    typedef typename MipMapCoverageAccumulator<T>::Type CoverageAccumulator;

    const int dstWidth = Max(width >> 1, 1);
    const int dstHeight = Max(height >> 1, 1);
    const int dstDepth = Max(depth >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    const size_t yOff = height < 2 ? 0 : static_cast<size_t>(numComponents) * width;
    const size_t zOff = depth < 2 ? 0 : static_cast<size_t>(numComponents) * width * height;
    const size_t xRemainder = width > 1 && (width & 1) ? numComponents : 0;
    const size_t yRemainder = height > 1 && (height & 1) ? static_cast<size_t>(numComponents) * width : 0;
    CoverageAccumulator a0 = 0;
    CoverageAccumulator a1 = 0;
    CoverageAccumulator a2 = 0;
    CoverageAccumulator a3 = 0;
    CoverageAccumulator a4 = 0;
    CoverageAccumulator a5 = 0;
    CoverageAccumulator a6 = 0;
    CoverageAccumulator a7 = 0;
    CoverageAccumulator alphaSum = 0;

    for (int z = 0; z < dstDepth; z++) {
        for (int y = 0; y < dstHeight; y++) {
            for (int x = 0; x < dstWidth; x++) {
                if (alphaComponentIndex >= 0) {
                    a0 = MipMapSample(src[alphaComponentIndex]);
                    a1 = MipMapSample(src[alphaComponentIndex + xOff]);
                    a2 = MipMapSample(src[alphaComponentIndex + yOff]);
                    a3 = MipMapSample(src[alphaComponentIndex + yOff + xOff]);
                    a4 = MipMapSample(src[alphaComponentIndex + zOff]);
                    a5 = MipMapSample(src[alphaComponentIndex + zOff + xOff]);
                    a6 = MipMapSample(src[alphaComponentIndex + zOff + yOff]);
                    a7 = MipMapSample(src[alphaComponentIndex + zOff + yOff + xOff]);
                    alphaSum = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
                }

                for (int i = 0; i < numComponents; i++) {
                    if (preserveCoverage && alphaSum > 0) {
                        if (i != alphaComponentIndex) {
                            CoverageAccumulator p0 = MipMapSample(src[0]) * a0;
                            CoverageAccumulator p1 = MipMapSample(src[xOff]) * a1;
                            CoverageAccumulator p2 = MipMapSample(src[yOff]) * a2;
                            CoverageAccumulator p3 = MipMapSample(src[yOff + xOff]) * a3;
                            CoverageAccumulator p4 = MipMapSample(src[zOff]) * a4;
                            CoverageAccumulator p5 = MipMapSample(src[zOff + xOff]) * a5;
                            CoverageAccumulator p6 = MipMapSample(src[zOff + yOff]) * a6;
                            CoverageAccumulator p7 = MipMapSample(src[zOff + yOff + xOff]) * a7;
                            *dst++ = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / alphaSum;
                        } else {
                            *dst++ = Max(Max4(a0, a1, a2, a3), Max4(a4, a5, a6, a7));
                        }
                    } else {
                        Accumulator p0 = MipMapSample(src[0]);
                        Accumulator p1 = MipMapSample(src[xOff]);
                        Accumulator p2 = MipMapSample(src[yOff]);
                        Accumulator p3 = MipMapSample(src[yOff + xOff]);
                        Accumulator p4 = MipMapSample(src[zOff]);
                        Accumulator p5 = MipMapSample(src[zOff + xOff]);
                        Accumulator p6 = MipMapSample(src[zOff + yOff]);
                        Accumulator p7 = MipMapSample(src[zOff + yOff + xOff]);
                        *dst++ = MipMapStore<T>((p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / static_cast<Accumulator>(8));
                    }
                    src++;
                }
                src += xOff;
            }
            src += xRemainder + yOff;
        }
        src += yRemainder + zOff;
    }
}

template <typename T>
static void BuildMipMap(T *dst, const T *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    if (depth > 1) {
        BuildMipMap3D(dst, src, width, height, depth, numComponents, alphaComponentIndex, preserveCoverage);
    } else if (height > 1) {
        BuildMipMap2D(dst, src, width, height, numComponents, alphaComponentIndex, preserveCoverage);
    } else {
        BuildMipMap1D(dst, src, width, numComponents, alphaComponentIndex, preserveCoverage);
    }
}

static void BuildMipMap1DWithGamma(byte *dst, const byte *src, int width, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int dstWidth = Max(width >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    float a0 = 0;
    float a1 = 0;
    float alphaSum = 0;

    for (int x = 0; x < dstWidth; x++) {
        if (alphaComponentIndex >= 0) {
            a0 = src[alphaComponentIndex];
            a1 = src[alphaComponentIndex + xOff];
            alphaSum = a0 + a1;
        }

        for (int i = 0; i < numComponents; i++) {
            if (preserveCoverage && alphaSum > 0) {
                if (i != alphaComponentIndex) {
                    float p0 = gammaToLinear[src[0]] * a0;
                    float p1 = gammaToLinear[src[xOff]] * a1;
                    *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1) / alphaSum));
                } else {
                    *dst++ = Max(a0, a1);
                }
            } else {
                if (i != alphaComponentIndex) {
                    float p0 = gammaToLinear[src[0]];
                    float p1 = gammaToLinear[src[xOff]];
                    *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1) * 0.5f));
                } else {
                    *dst++ = (a0 + a1) * 0.5f;
                }
            }
            src++;
        }
        src += xOff;
    }
}

static void BuildMipMap2DWithGamma(byte *dst, const byte *src, int width, int height, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int dstWidth = Max(width >> 1, 1);
    const int dstHeight = Max(height >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    const size_t yOff = height < 2 ? 0 : static_cast<size_t>(numComponents) * width;
    const size_t xRemainder = width > 1 && (width & 1) ? numComponents : 0;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float alphaSum = 0;

    for (int y = 0; y < dstHeight; y++) {
        for (int x = 0; x < dstWidth; x++) {
            if (alphaComponentIndex >= 0) {
                a0 = src[alphaComponentIndex];
                a1 = src[alphaComponentIndex + xOff];
                a2 = src[alphaComponentIndex + yOff];
                a3 = src[alphaComponentIndex + yOff + xOff];
                alphaSum = a0 + a1 + a2 + a3;
            }

            for (int i = 0; i < numComponents; i++) {
                if (preserveCoverage && alphaSum > 0) {
                    if (i != alphaComponentIndex) {
                        float p0 = gammaToLinear[src[0]] * a0;
                        float p1 = gammaToLinear[src[xOff]] * a1;
                        float p2 = gammaToLinear[src[yOff]] * a2;
                        float p3 = gammaToLinear[src[yOff + xOff]] * a3;
                        *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3) / alphaSum));
                    } else {
                        *dst++ = Max4(a0, a1, a2, a3);
                    }
                } else {
                    if (i != alphaComponentIndex) {
                        float p0 = gammaToLinear[src[0]];
                        float p1 = gammaToLinear[src[xOff]];
                        float p2 = gammaToLinear[src[yOff]];
                        float p3 = gammaToLinear[src[yOff + xOff]];
                        *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3) * 0.25f));
                    } else {
                        *dst++ = (a0 + a1 + a2 + a3) * 0.25f;
                    }
                }
                src++;
            }
            src += xOff;
        }
        src += xRemainder + yOff;
    }
}

static void BuildMipMap3DWithGamma(byte *dst, const byte *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int dstWidth = Max(width >> 1, 1);
    const int dstHeight = Max(height >> 1, 1);
    const int dstDepth = Max(depth >> 1, 1);
    const size_t xOff = width < 2 ? 0 : numComponents;
    const size_t yOff = height < 2 ? 0 : static_cast<size_t>(numComponents) * width;
    const size_t zOff = depth < 2 ? 0 : static_cast<size_t>(numComponents) * width * height;
    const size_t xRemainder = width > 1 && (width & 1) ? numComponents : 0;
    const size_t yRemainder = height > 1 && (height & 1) ? static_cast<size_t>(numComponents) * width : 0;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float a4 = 0;
    float a5 = 0;
    float a6 = 0;
    float a7 = 0;
    float alphaSum = 0;

    for (int z = 0; z < dstDepth; z++) {
        for (int y = 0; y < dstHeight; y++) {
            for (int x = 0; x < dstWidth; x++) {
                if (alphaComponentIndex >= 0) {
                    a0 = src[alphaComponentIndex];
                    a1 = src[alphaComponentIndex + xOff];
                    a2 = src[alphaComponentIndex + yOff];
                    a3 = src[alphaComponentIndex + yOff + xOff];
                    a4 = src[alphaComponentIndex + zOff];
                    a5 = src[alphaComponentIndex + zOff + xOff];
                    a6 = src[alphaComponentIndex + zOff + yOff];
                    a7 = src[alphaComponentIndex + zOff + yOff + xOff];
                    alphaSum = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
                }

                for (int i = 0; i < numComponents; i++) {
                    if (preserveCoverage && alphaSum > 0) {
                        if (i != alphaComponentIndex) {
                            float p0 = gammaToLinear[src[0]] * a0;
                            float p1 = gammaToLinear[src[xOff]] * a1;
                            float p2 = gammaToLinear[src[yOff]] * a2;
                            float p3 = gammaToLinear[src[yOff + xOff]] * a3;
                            float p4 = gammaToLinear[src[zOff]] * a4;
                            float p5 = gammaToLinear[src[zOff + xOff]] * a5;
                            float p6 = gammaToLinear[src[zOff + yOff]] * a6;
                            float p7 = gammaToLinear[src[zOff + yOff + xOff]] * a7;
                            *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / alphaSum));
                        } else {
                            *dst++ = Max(Max4(a0, a1, a2, a3), Max4(a4, a5, a6, a7));
                        }
                    } else {
                        if (i != alphaComponentIndex) {
                            float p0 = gammaToLinear[src[0]];
                            float p1 = gammaToLinear[src[xOff]];
                            float p2 = gammaToLinear[src[yOff]];
                            float p3 = gammaToLinear[src[yOff + xOff]];
                            float p4 = gammaToLinear[src[zOff]];
                            float p5 = gammaToLinear[src[zOff + xOff]];
                            float p6 = gammaToLinear[src[zOff + yOff]];
                            float p7 = gammaToLinear[src[zOff + yOff + xOff]];
                            *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) * 0.125f));
                        } else {
                            *dst++ = (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7) * 0.125f;
                        }
                    }
                    src++;
                }
                src += xOff;
            }
            src += xRemainder + yOff;
        }
        src += yRemainder + zOff;
    }
}

static void BuildMipMapWithGamma(byte *dst, const byte *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    if (depth > 1) {
        BuildMipMap3DWithGamma(dst, src, width, height, depth, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    } else if (height > 1) {
        BuildMipMap2DWithGamma(dst, src, width, height, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    } else {
        BuildMipMap1DWithGamma(dst, src, width, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    }
}

static int GetAlphaComponentIndex(Image::Format format) {
    switch (format) {
    case Image::Format::A8:
    case Image::Format::A16_FLOAT:
    case Image::Format::A32_FLOAT:
    case Image::Format::A8B8G8R8:
    case Image::Format::A8R8G8B8:
        return 0;
    case Image::Format::L8A8:
    case Image::Format::L16A16_FLOAT:
    case Image::Format::L32A32_FLOAT:
        return 1;
    case Image::Format::R8G8B8A8:
    case Image::Format::R8G8B8A8_SNORM:
    case Image::Format::R8G8B8A8_SINT:
    case Image::Format::R8G8B8A8_UINT:
    case Image::Format::B8G8R8A8:
    case Image::Format::R16G16B16A16_FLOAT:
    case Image::Format::R16G16B16A16:
    case Image::Format::R16G16B16A16_SNORM:
    case Image::Format::R16G16B16A16_SINT:
    case Image::Format::R16G16B16A16_UINT:
    case Image::Format::R32G32B32A32_FLOAT:
    case Image::Format::R32G32B32A32_SINT:
    case Image::Format::R32G32B32A32_UINT:
        return 3;
    default:
        return -1;
    }
}

Image &Image::GenerateMipmaps(bool preserveAlphaCoverage) {
    if (IsEmpty() || numMipLevels <= 1) {
        return *this;
    }

    if (IsCompressed()) {
        BE_WARNLOG("Image::GenerateMipmaps: Couldn't generate mipmaps for a compressed image.\n");
        return *this;
    }

    if (IsPacked()) {
        BE_WARNLOG("Image::GenerateMipmaps: Couldn't generate mipmaps for a packed format image.\n");
        return *this;
    }

    if (IsDepthFormat() || IsDepthStencilFormat()) {
        BE_WARNLOG("Image::GenerateMipmaps: Couldn't generate mipmaps for a depth/stencil image.\n");
        return *this;
    }

    const ImageFormatInfo *formatInfo = GetImageFormatInfo(format);
    const int numComponents = NumComponents();
    const int bytesPerPixel = BytesPerPixel();
    if (numComponents <= 0 || bytesPerPixel <= 0 || bytesPerPixel % numComponents != 0) {
        BE_WARNLOG("Image::GenerateMipmaps: Unsupported format %s.\n", FormatName());
        return *this;
    }

    const int bytesPerComponent = bytesPerPixel / numComponents;
    const bool isSigned = HasFlag(formatInfo->type, FormatType::SNorm) || HasFlag(formatInfo->type, FormatType::SInt);
    const bool useGammaLut = bytesPerComponent == 1 && HasFlag(formatInfo->type, FormatType::UNorm) && (gammaSpace == GammaSpace::sRGB || gammaSpace == GammaSpace::Pow22);
    if (!IsFloatFormat() && bytesPerComponent != 1 && bytesPerComponent != 2 && bytesPerComponent != 4) {
        BE_WARNLOG("Image::GenerateMipmaps: Unsupported component size for format %s.\n", FormatName());
        return *this;
    }

    preserveAlphaCoverage = preserveAlphaCoverage && HasAlpha();

    const int alphaComponentIndex = GetAlphaComponentIndex(format);
    const int numSlices = GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 1; mipLevel < numMipLevels; mipLevel++) {
            const int srcMipLevel = mipLevel - 1;
            const int w = GetWidth(srcMipLevel);
            const int h = GetHeight(srcMipLevel);
            const int d = GetDepth(srcMipLevel);

            const byte *src = GetPixels(srcMipLevel, sliceIndex);
            byte *dst = GetPixels(mipLevel, sliceIndex);

            if (IsFloatFormat()) {
                if (IsHalfFormat()) {
                    BuildMipMap<half>((half *)dst, (const half *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                } else {
                    BuildMipMap<float>((float *)dst, (const float *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            } else if (useGammaLut) { // bytesPerComponent == 1 && UNorm && (gammaSpace == sRGB || gammaSpace == Pow22)
                if (gammaSpace == GammaSpace::sRGB) {
                    BuildMipMapWithGamma(dst, src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage, Image::sRGBToLinearTable, Image::LinearToGammaApprox);
                } else {
                    BuildMipMapWithGamma(dst, src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage, Image::pow22ToLinearTable, Image::LinearToGammaFast);
                }
            } else if (bytesPerComponent == 1) {
                if (isSigned) {
                    BuildMipMap<int8_t>((int8_t *)dst, (const int8_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                } else {
                    BuildMipMap<uint8_t>((uint8_t *)dst, (const uint8_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            } else if (bytesPerComponent == 2) {
                if (isSigned) {
                    BuildMipMap<int16_t>((int16_t *)dst, (const int16_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                } else {
                    BuildMipMap<uint16_t>((uint16_t *)dst, (const uint16_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            } else if (bytesPerComponent == 4) {
                if (isSigned) {
                    BuildMipMap<int32_t>((int32_t *)dst, (const int32_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                } else {
                    BuildMipMap<uint32_t>((uint32_t *)dst, (const uint32_t *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            }
        }
    }
    return *this;
}

BE_NAMESPACE_END
