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

#include "BlueshiftEngine.h"
#include "Test.h"

#include <cstdio>
#include <cstdlib>

static void VerifyImageTest(bool expression, const char *testName, int line) {
    if (!expression) {
        std::fprintf(stderr, "%s verification failed at line %d\n", testName, line);
        BE_FATALERROR("%s verification failed at line %d\n", testName, line);
        std::abort();
    }
}

#define VERIFY_IMAGE(expression) VerifyImageTest((expression), __func__, __LINE__)

static void TestImageFormatSizes() {
    VERIFY_IMAGE(BE1::Image::BytesPerPixel(BE1::Image::Format::R8G8B8A8_SNORM) == 4);
    VERIFY_IMAGE(BE1::Image::BytesPerPixel(BE1::Image::Format::R8G8B8A8_SINT) == 4);
    VERIFY_IMAGE(BE1::Image::BytesPerPixel(BE1::Image::Format::R8G8B8A8_UINT) == 4);
    VERIFY_IMAGE(BE1::Image::BytesPerPixel(BE1::Image::Format::D24X8) == 4);
}

static void TestImageMipAndSliceAddressing() {
    BE1::Image image;
    image.Create2DArray(2, 2, 2, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    byte *base = image.GetPixels();
    VERIFY_IMAGE(image.GetPixels(1) == base + 4);
    VERIFY_IMAGE(image.GetPixels(0, 1) == base + 5);
    VERIFY_IMAGE(image.GetPixels(1, 1) == base + 9);
}

static void TestImagePointerPreservation() {
    uint16_t gammaRamp[768];
    for (int i = 0; i < 256; i++) {
        gammaRamp[i] = static_cast<uint16_t>(i * 257);
        gammaRamp[i + 256] = static_cast<uint16_t>(i * 257);
        gammaRamp[i + 512] = static_cast<uint16_t>(i * 257);
    }

    const byte rgb[] = { 10, 20, 30, 40, 50, 60 };
    BE1::Image rgbImage;
    rgbImage.Create2D(2, 1, 1, BE1::Image::Format::R8G8B8, BE1::Image::GammaSpace::Linear, rgb, BE1::Image::Flag::None);
    byte *rgbPixels = rgbImage.GetPixels();
    rgbImage.ApplyGammaRampTableRGB888(gammaRamp);
    VERIFY_IMAGE(rgbImage.GetPixels() == rgbPixels);
    VERIFY_IMAGE(rgbPixels[0] == 10 && rgbPixels[5] == 60);

    const byte rgba[] = { 10, 20, 30, 40 };
    BE1::Image rgbaImage;
    rgbaImage.Create2D(1, 1, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::Linear, rgba, BE1::Image::Flag::None);
    byte *rgbaPixels = rgbaImage.GetPixels();
    rgbaImage.SwapRedAlphaRGBA8888();
    VERIFY_IMAGE(rgbaImage.GetPixels() == rgbaPixels);
    VERIFY_IMAGE(rgbaPixels[0] == 40 && rgbaPixels[1] == 20 && rgbaPixels[2] == 30 && rgbaPixels[3] == 10);
}

static void TestImageOddMipGeneration() {
    byte guardedPixels[12];
    for (byte &value : guardedPixels) {
        value = 0xCD;
    }
    for (int i = 1; i <= 9; i++) {
        guardedPixels[i] = 20;
    }

    BE1::Image image;
    image.InitFromMemory(3, 3, 1, 1, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, guardedPixels + 1, BE1::Image::Flag::None);
    image.GenerateMipmaps();

    VERIFY_IMAGE(guardedPixels[0] == 0xCD);
    VERIFY_IMAGE(guardedPixels[10] == 20);
    VERIFY_IMAGE(guardedPixels[11] == 0xCD);
}

static void TestImageMipSampling() {
    BE1::Image image;
    image.Create2D(4, 4, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    memset(image.GetPixels(0), 0, image.SizeInBytes(0, 1));
    memset(image.GetPixels(1), 255, image.SizeInBytes(1, 1));

    const BE1::Color4 color = image.Sample2D(BE1::Vec2(0.5f, 0.5f), BE1::Image::SampleWrapMode::Clamp, BE1::Image::SampleWrapMode::Clamp, BE1::Image::SampleFilter::Bilinear, 1);
    VERIFY_IMAGE(std::fabs(color[0] - 1.0f) < 1e-6f);
}

static void TestImageArrayFlip() {
    const byte pixels[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    BE1::Image flipX;
    flipX.Create2DArray(2, 2, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels, BE1::Image::Flag::None);
    flipX.FlipX();
    const byte expectedX[] = { 2, 1, 4, 3, 6, 5, 8, 7 };
    VERIFY_IMAGE(memcmp(flipX.GetPixels(), expectedX, sizeof(expectedX)) == 0);

    BE1::Image flipY;
    flipY.Create2DArray(2, 2, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels, BE1::Image::Flag::None);
    flipY.FlipY();
    const byte expectedY[] = { 3, 4, 1, 2, 7, 8, 5, 6 };
    VERIFY_IMAGE(memcmp(flipY.GetPixels(), expectedY, sizeof(expectedY)) == 0);
}

static void TestImageResize() {
    const byte pixels[] = { 1, 2, 3, 4 };
    BE1::Image arrayImage;
    arrayImage.Create2DArray(2, 1, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels, BE1::Image::Flag::None);

    BE1::Image sameSize;
    VERIFY_IMAGE(arrayImage.Resize(2, 1, BE1::Image::ResampleFilter::Nearest, sameSize));
    VERIFY_IMAGE(!sameSize.IsEmpty());
    VERIFY_IMAGE(sameSize.GetArraySize() == 2);
    VERIFY_IMAGE(memcmp(sameSize.GetPixels(), pixels, sizeof(pixels)) == 0);

    BE1::Image resizedArray;
    VERIFY_IMAGE(arrayImage.Resize(4, 1, BE1::Image::ResampleFilter::Nearest, resizedArray));
    const byte expectedArray[] = { 1, 1, 2, 2, 3, 3, 4, 4 };
    VERIFY_IMAGE(resizedArray.GetArraySize() == 2);
    VERIFY_IMAGE(memcmp(resizedArray.GetPixels(), expectedArray, sizeof(expectedArray)) == 0);

    const uint16_t r16Pixels[] = { 0x1234, 0xABCD };
    BE1::Image r16Image;
    r16Image.Create2D(2, 1, 1, BE1::Image::Format::R16, BE1::Image::GammaSpace::Linear, reinterpret_cast<const byte *>(r16Pixels), BE1::Image::Flag::None);
    BE1::Image resizedR16;
    VERIFY_IMAGE(r16Image.Resize(4, 1, BE1::Image::ResampleFilter::Nearest, resizedR16));
    const uint16_t *resizedR16Pixels = reinterpret_cast<const uint16_t *>(resizedR16.GetPixels());
    VERIFY_IMAGE(resizedR16Pixels[0] == 0x1234 && resizedR16Pixels[1] == 0x1234);
    VERIFY_IMAGE(resizedR16Pixels[2] == 0xABCD && resizedR16Pixels[3] == 0xABCD);
}

static void TestImageGammaConversionAndMipRegeneration() {
    const byte rgba[] = { 64, 96, 128, 128 };
    BE1::Image source;
    source.Create2D(1, 1, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, rgba, BE1::Image::Flag::None);

    BE1::Image linear;
    VERIFY_IMAGE(source.ConvertFormat(BE1::Image::Format::R8G8B8A8, linear, BE1::Image::GammaSpace::Linear));
    VERIFY_IMAGE(linear.GetPixels()[3] == 128);

    byte basePixels[16];
    memset(basePixels, 80, sizeof(basePixels));
    BE1::Image oneMip;
    oneMip.Create2D(4, 4, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, basePixels, BE1::Image::Flag::None);
    BE1::Image mipmapped;
    VERIFY_IMAGE(oneMip.ConvertFormat(BE1::Image::Format::R8, mipmapped, BE1::Image::GammaSpace::Linear, BE1::Image::MipmapGenerationMode::Mipmaps));
    VERIFY_IMAGE(mipmapped.NumMipmaps() == 3);
    VERIFY_IMAGE(mipmapped.GetPixels(1)[0] == 80 && mipmapped.GetPixels(2)[0] == 80);
}

#undef VERIFY_IMAGE

void Test::TestImage() {
    TestImageFormatSizes();
    TestImageMipAndSliceAddressing();
    TestImagePointerPreservation();
    TestImageOddMipGeneration();
    TestImageMipSampling();
    TestImageArrayFlip();
    TestImageResize();
    TestImageGammaConversionAndMipRegeneration();
}
