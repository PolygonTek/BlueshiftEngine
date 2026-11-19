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
    image.Create2DArray(2, 2, 2, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, nullptr);

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
    rgbImage.Create2D(2, 1, 1, BE1::Image::Format::R8G8B8, BE1::Image::GammaSpace::Linear, rgb);
    byte *rgbPixels = rgbImage.GetPixels();
    rgbImage.ApplyGammaRampTableRGB888(gammaRamp);
    VERIFY_IMAGE(rgbImage.GetPixels() == rgbPixels);
    VERIFY_IMAGE(rgbPixels[0] == 10 && rgbPixels[5] == 60);

    const byte rgba[] = { 10, 20, 30, 40 };
    BE1::Image rgbaImage;
    rgbaImage.Create2D(1, 1, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::Linear, rgba);
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
    image.Init2DFromMemory(3, 3, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, guardedPixels + 1);
    VERIFY_IMAGE(image.GenerateMipmaps());

    VERIFY_IMAGE(guardedPixels[0] == 0xCD);
    VERIFY_IMAGE(guardedPixels[10] == 20);
    VERIFY_IMAGE(guardedPixels[11] == 0xCD);
}

static void TestImageMipSampling() {
    BE1::Image image;
    image.Create2D(4, 4, 2, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, nullptr);
    memset(image.GetPixels(0), 0, image.SizeInBytes(0, 1));
    memset(image.GetPixels(1), 255, image.SizeInBytes(1, 1));

    const BE1::Color4 color = image.Sample2D(BE1::Vec2(0.5f, 0.5f), BE1::Image::SampleWrapMode::Clamp, BE1::Image::SampleWrapMode::Clamp, BE1::Image::SampleFilter::Bilinear, 1);
    VERIFY_IMAGE(std::fabs(color[0] - 1.0f) < 1e-6f);
}

static void TestImageArrayFlip() {
    const byte pixels[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    BE1::Image flipX;
    flipX.Create2DArray(2, 2, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels);
    flipX.FlipX();
    const byte expectedX[] = { 2, 1, 4, 3, 6, 5, 8, 7 };
    VERIFY_IMAGE(memcmp(flipX.GetPixels(), expectedX, sizeof(expectedX)) == 0);

    BE1::Image flipY;
    flipY.Create2DArray(2, 2, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels);
    flipY.FlipY();
    const byte expectedY[] = { 3, 4, 1, 2, 7, 8, 5, 6 };
    VERIFY_IMAGE(memcmp(flipY.GetPixels(), expectedY, sizeof(expectedY)) == 0);
}

static void TestImageResize() {
    const byte pixels[] = { 1, 2, 3, 4 };
    BE1::Image arrayImage;
    arrayImage.Create2DArray(2, 1, 2, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, pixels);

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
    r16Image.Create2D(2, 1, 1, BE1::Image::Format::R16, BE1::Image::GammaSpace::Linear, reinterpret_cast<const byte *>(r16Pixels));
    BE1::Image resizedR16;
    VERIFY_IMAGE(r16Image.Resize(4, 1, BE1::Image::ResampleFilter::Nearest, resizedR16));
    const uint16_t *resizedR16Pixels = reinterpret_cast<const uint16_t *>(resizedR16.GetPixels());
    VERIFY_IMAGE(resizedR16Pixels[0] == 0x1234 && resizedR16Pixels[1] == 0x1234);
    VERIFY_IMAGE(resizedR16Pixels[2] == 0xABCD && resizedR16Pixels[3] == 0xABCD);
}

static void TestImageGammaConversionAndMipRegeneration() {
    const byte rgba[] = { 64, 96, 128, 128 };
    BE1::Image source;
    source.Create2D(1, 1, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, rgba);

    BE1::Image linear;
    VERIFY_IMAGE(source.ConvertFormat(BE1::Image::Format::R8G8B8A8, linear, BE1::Image::GammaSpace::Linear));
    VERIFY_IMAGE(linear.GetPixels()[3] == 128);

    byte basePixels[16];
    memset(basePixels, 80, sizeof(basePixels));
    BE1::Image oneMip;
    oneMip.Create2D(4, 4, 1, BE1::Image::Format::R8, BE1::Image::GammaSpace::Linear, basePixels);
    BE1::Image mipmapped;
    VERIFY_IMAGE(oneMip.ConvertFormat(BE1::Image::Format::R8, mipmapped, BE1::Image::GammaSpace::Linear, BE1::Image::MipmapGenerationMode::Mipmaps));
    VERIFY_IMAGE(mipmapped.NumMipmaps() == 3);
    VERIFY_IMAGE(mipmapped.GetPixels(1)[0] == 80 && mipmapped.GetPixels(2)[0] == 80);
}

static void TestImageBTEXMipChain() {
    using Image = BE1::Image;
    BE1::Str filename = BE1::fileSystem.GetBaseDir();
    filename.AppendPath(BE1::Str("btex_mip_chain_test_") + BE1::Guid::CreateGuid().ToString() + ".btex");

    // A single mip and a partial chain are both final BTEX data, just like a complete chain.
    for (int mipCount = 1; mipCount <= 3; mipCount++) {
        Image source;
        const Image::Flag retainedFlags = Image::Flag::Volume;
        const uint32_t retiredBits = BIT(1) | BIT(2) | BIT(3) | BIT(4);
        source.Create3D(3, 5, 2, mipCount, Image::Format::R8G8B8A8, Image::GammaSpace::Linear, nullptr, retainedFlags | static_cast<Image::Flag>(retiredBits));
        for (int mip = 0; mip < mipCount; mip++) {
            memset(source.GetPixels(mip), 41 + mip, source.SizeInBytes(mip, 1));
        }
        VERIFY_IMAGE(source.WriteBTEX(filename));

        byte *data = nullptr;
        const size_t size = BE1::fileSystem.LoadFile(filename, true, reinterpret_cast<void **>(&data));
        VERIFY_IMAGE(data != nullptr && size > 32);
        uint32_t diskFlags;
        memcpy(&diskFlags, data + 12, sizeof(diskFlags));
        BE1::ByteOrder::LittleEndianToSystem(diskFlags);
        VERIFY_IMAGE(diskFlags == static_cast<uint32_t>(retainedFlags));

        // Emulate an existing v1 asset carrying retired texture hints without changing its payload.
        diskFlags |= retiredBits;
        BE1::ByteOrder::SystemToLittleEndian(diskFlags);
        memcpy(data + 12, &diskFlags, sizeof(diskFlags));
        BE1::File *fp = BE1::fileSystem.OpenFile(filename, BE1::File::Mode::Write);
        VERIFY_IMAGE(fp != nullptr);
        VERIFY_IMAGE(fp->Write(data, size));
        BE1::fileSystem.CloseFile(fp);
        BE1::fileSystem.FreeFile(data);

        Image loaded;
        VERIFY_IMAGE(loaded.Load(filename));
        VERIFY_IMAGE(loaded.GetFlags() == retainedFlags);
        VERIFY_IMAGE(loaded.GetFormat() == source.GetFormat());
        VERIFY_IMAGE(loaded.NumMipmaps() == mipCount);
        VERIFY_IMAGE(loaded.GetWidth() == 3 && loaded.GetHeight() == 5 && loaded.GetDepth() == 2);
        VERIFY_IMAGE(loaded.SizeInBytes(0, mipCount) == source.SizeInBytes(0, mipCount));
        VERIFY_IMAGE(memcmp(loaded.GetPixels(), source.GetPixels(), source.SizeInBytes(0, mipCount)) == 0);

        Image::MipRange range;
        VERIFY_IMAGE(loaded.LoadMipRange(filename, 0, mipCount, &range));
        VERIFY_IMAGE(loaded.GetFlags() == retainedFlags && loaded.NumMipmaps() == mipCount);
        VERIFY_IMAGE(range.originalMipLevels == mipCount && range.firstMip == 0);

        VERIFY_IMAGE(loaded.LoadMipRange(filename, mipCount - 1, 1, &range));
        VERIFY_IMAGE(loaded.GetFlags() == retainedFlags && loaded.NumMipmaps() == 1);
        VERIFY_IMAGE(range.firstMip == mipCount - 1 && range.originalMipLevels == mipCount);
        VERIFY_IMAGE(loaded.SizeInBytes() == source.SizeInBytes(mipCount - 1, 1));
        VERIFY_IMAGE(memcmp(loaded.GetPixels(), source.GetPixels(mipCount - 1), loaded.SizeInBytes()) == 0);

        VERIFY_IMAGE(loaded.LoadMipRangeForMaxDimension(filename, 1, &range));
        VERIFY_IMAGE(loaded.GetFlags() == retainedFlags && loaded.Is3D());
        VERIFY_IMAGE(range.firstMip == mipCount - 1 && loaded.NumMipmaps() == 1);
    }
    VERIFY_IMAGE(BE1::fileSystem.RemoveFile(filename, false));
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
    TestImageBTEXMipChain();
}
