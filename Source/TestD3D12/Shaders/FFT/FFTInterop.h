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

#ifndef __FFT_INTEROP_H__
#define __FFT_INTEROP_H__

#include "../ShaderTypes.h"

static const uint MultiplySpectrum2DBlockSize = 256;

static const uint FFTRadix2_256StageCount = 8;
static const uint FFTRadix2_256ButterflyCount = 256 / 2;
static const uint FFTRadix2_256BlockSize = FFTRadix2_256ButterflyCount;
static const uint FFTRadix4_256StageCount = 4;
static const uint FFTRadix4_256ButterflyCount = 256 / 4;
static const uint FFTRadix4_256BlockSize = FFTRadix4_256ButterflyCount;
static const uint FFTRadix8_512StageCount = 3;
static const uint FFTRadix8_512ButterflyCount = 512 / 8;
static const uint FFTRadix8_512BlockSize = FFTRadix8_512ButterflyCount;
static const uint FFTRadix8_1024StageCount = 3;
static const uint FFTRadix8_1024ButterflyCount = 1024 / 8;
static const uint FFTRadix8_1024BlockSize = FFTRadix8_1024ButterflyCount;
static const uint FFTRadix2_1024ButterflyCount = 1024 / 2;
static const uint FFTRadix2_1024BlockSize = FFTRadix2_1024ButterflyCount;

struct MultiplySpectrum2DParams {
    uint fftSize;
    uint planeCount;
    float scale;
};

#ifdef __cplusplus
static_assert(sizeof(MultiplySpectrum2DParams) == 12, "MultiplySpectrum2DParams must match the three HLSL root constants");
#endif

struct FFTStageParams {
    float sign;
    float scale;
};

#endif // __FFT_INTEROP_H__
