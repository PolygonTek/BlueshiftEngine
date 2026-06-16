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
#include "RenderCvars.h"

CVAR(r_skipCulling, "0", BE1::CVar::Flag::Bool, "skip frustum culling for render objects and mesh surfaces");
CVAR(r_skipBackend, "0", BE1::CVar::Flag::Bool, "skip render backend command execution");

CVAR(r_textureStreamingStreamInDelay, "0", BE1::CVar::Flag::Integer, "texture streaming frame delay before loading higher-resolution mips");
CVAR(r_textureStreamingMaxRequestsPerFrame, "256", BE1::CVar::Flag::Integer, "maximum texture streaming requests per frame");
CVAR(r_textureStreamingMaxLevelReduction, "6", BE1::CVar::Flag::Integer, "maximum mip level reduction for streaming textures");
CVAR(r_textureStreamingMinResidentMaxDimension, "64", BE1::CVar::Flag::Integer, "minimum requested resident texture max dimension");

CVAR(r_toneMapping, "1", BE1::CVar::Flag::Bool, "enable compute tone mapping");
CVAR_MINMAX(r_toneMapOp, "7", BE1::CVar::Flag::Integer, "tone mapping operator", 0, 10);
CVAR(r_toneMapLuminanceLogMin, "-10", BE1::CVar::Flag::Float, "tone mapping luminance histogram minimum log2 luminance");
CVAR(r_toneMapLuminanceLogMax, "2", BE1::CVar::Flag::Float, "tone mapping luminance histogram maximum log2 luminance");
CVAR_MINMAX(r_toneMapLuminanceAdaptationRate, "1.1", BE1::CVar::Flag::Float, "tone mapping luminance adaptation rate", 0.0f, 100.0f);
CVAR_MINMAX(r_toneMapMiddleGray, "0.18", BE1::CVar::Flag::Float, "tone mapping middle gray", 0.0f, 10.0f);

CVAR(r_bloom, "1", BE1::CVar::Flag::Bool, "enable bloom");
CVAR_MINMAX(r_bloomThreshold, "1.0", BE1::CVar::Flag::Float, "bloom luminance threshold", 0.0f, 100.0f);
CVAR_MINMAX(r_bloomSoftThreshold, "0.5", BE1::CVar::Flag::Float, "bloom soft threshold width", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomPrefilterQuality, "1", BE1::CVar::Flag::Integer, "bloom prefilter quality (0 = low quality, 1 = high quality)", 0, 1);
CVAR_MINMAX(r_bloomIntensity, "0.15", BE1::CVar::Flag::Float, "final bloom composite intensity", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomScatter, "0.85", BE1::CVar::Flag::Float, "bloom mip scatter weight", 0.0f, 1.0f);
CVAR_MINMAX(r_bloomRadius, "1.0", BE1::CVar::Flag::Float, "bloom dual kawase sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomMipCount, "6", BE1::CVar::Flag::Integer, "bloom mip level count", 1, 8);
