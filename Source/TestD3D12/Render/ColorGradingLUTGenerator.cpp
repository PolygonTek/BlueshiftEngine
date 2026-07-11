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
#include "ColorGradingLUTGenerator.h"

void ColorGradingLUTGenerator::Generate(const Settings &settings, BE1::Image &outImage) {
    // outImage 가 재사용 가능한 image 인지 판단
    const bool reuseImage = !outImage.IsEmpty() &&
        outImage.GetWidth() == LUTSize &&
        outImage.GetHeight() == LUTSize &&
        outImage.GetDepth() == LUTSize &&
        outImage.GetArraySize() == 1 &&
        outImage.NumMipmaps() == 1 &&
        outImage.GetFormat() == BE1::Image::Format::R16G16B16A16_FLOAT &&
        outImage.GetFlags() == BE1::Image::Flag::None &&
        outImage.GetGammaSpace() == BE1::Image::GammaSpace::Linear;
    if (!reuseImage) {
        outImage.Create3D(LUTSize, LUTSize, LUTSize, 1, BE1::Image::Format::R16G16B16A16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    }

    const float saturation = BE1::Max(settings.saturation, 0.0f);
    const float contrast = BE1::Max(settings.contrast, 0.0f);
    const float inverseGamma = 1.0f / BE1::Max(settings.gamma, 0.0001f);
    const float gain = BE1::Max(settings.gain, 0.0f);
    const BE1::Color3 gainTint(
        BE1::Max(settings.tint.r, 0.0f) * gain,
        BE1::Max(settings.tint.g, 0.0f) * gain,
        BE1::Max(settings.tint.b, 0.0f) * gain);
    const bool applyGamma = inverseGamma != 1.0f;
    const float inverseLUTMaxIndex = 1.0f / static_cast<float>(LUTSize - 1);
    const BE1::float16_t opaqueAlpha = BE1::F16Converter::FromF32(1.0f);

    BE1::float16_t *dst = reinterpret_cast<BE1::float16_t *>(outImage.GetPixels());

    for (int blueIndex = 0; blueIndex < LUTSize; blueIndex++) {
        const float blue = static_cast<float>(blueIndex) * inverseLUTMaxIndex;

        for (int greenIndex = 0; greenIndex < LUTSize; greenIndex++) {
            const float green = static_cast<float>(greenIndex) * inverseLUTMaxIndex;

            for (int redIndex = 0; redIndex < LUTSize; redIndex++) {
                const float red = static_cast<float>(redIndex) * inverseLUTMaxIndex;

                BE1::Color3 color(red, green, blue);
                const float luminance = BE1::Image::GetLuminance(color);

                // saturation & contrast 적용
                color = BE1::Color3(luminance) + (color - luminance) * saturation;
                color = (color - 0.5f) * contrast + 0.5f;

                color.r = BE1::Max(color.r, 0.0f);
                color.g = BE1::Max(color.g, 0.0f);
                color.b = BE1::Max(color.b, 0.0f);

                // gamma 적용
                if (applyGamma) {
                    color.r = BE1::Math::Pow(color.r, inverseGamma);
                    color.g = BE1::Math::Pow(color.g, inverseGamma);
                    color.b = BE1::Math::Pow(color.b, inverseGamma);
                }

                // color gain tint 적용
                color *= gainTint;
                color.r = BE1::Min(color.r, 1.0f);
                color.g = BE1::Min(color.g, 1.0f);
                color.b = BE1::Min(color.b, 1.0f);

                dst[0] = BE1::F16Converter::FromF32(color.r);
                dst[1] = BE1::F16Converter::FromF32(color.g);
                dst[2] = BE1::F16Converter::FromF32(color.b);
                dst[3] = opaqueAlpha;
                dst += 4;
            }
        }
    }
}
