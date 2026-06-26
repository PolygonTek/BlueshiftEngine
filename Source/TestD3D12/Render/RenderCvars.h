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

#pragma once

#include "Core/CVars.h"

enum RenderBloomMode {
    Off = 0,
    Pyramid = 1,
    Convolution = 2
};

extern BE1::CVar    r_skipCulling;
extern BE1::CVar    r_skipDrawCamera2D;
extern BE1::CVar    r_skipDrawCamera3D;
extern BE1::CVar    r_skipPostProcess;

extern BE1::CVar    r_textureStreamingStreamInDelay;
extern BE1::CVar    r_textureStreamingMaxRequestsPerFrame;
extern BE1::CVar    r_textureStreamingMaxLevelReduction;
extern BE1::CVar    r_textureStreamingMinResidentMaxDimension;

extern BE1::CVar    r_toneMapping;
extern BE1::CVar    r_toneMapOp;
extern BE1::CVar    r_toneMapLuminanceLogMin;
extern BE1::CVar    r_toneMapLuminanceLogMax;
extern BE1::CVar    r_toneMapLuminanceAdaptationRate;
extern BE1::CVar    r_toneMapMiddleGray;

extern BE1::CVar    r_bloom;
extern BE1::CVar    r_bloomIntensity;
extern BE1::CVar    r_bloomThreshold;
extern BE1::CVar    r_bloomSoftThreshold;

extern BE1::CVar    r_pyramidBloomPrefilterQuality;
extern BE1::CVar    r_pyramidBloomScatter;
extern BE1::CVar    r_pyramidBloomPrefilterRadius;
extern BE1::CVar    r_pyramidBloomDownsampleRadius;
extern BE1::CVar    r_pyramidBloomUpsampleRadius;
extern BE1::CVar    r_pyramidBloomMipCount;

extern BE1::CVar    r_convolutionBloomFFTMaxSize;
extern BE1::CVar    r_convolutionBloomKernelScale;
extern BE1::CVar    r_convolutionBloomKernelRotation;
