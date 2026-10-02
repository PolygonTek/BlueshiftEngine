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

#ifndef __GEOMETRY_CLIPMAP_INTEROP_H__
#define __GEOMETRY_CLIPMAP_INTEROP_H__

#ifdef __cplusplus
#include "ShaderTypes.h"
#endif

struct GeometryClipmapPatch {
    int2 gridOrigin;
    float gridSpacing;
    uint level;
    float morphStart;
    float morphScale;
    uint vertexResolution;
    uint packedBounds; // outer edge mask (L/R/B/T), width << 8, height << 16
};

#ifdef __cplusplus
static_assert(sizeof(GeometryClipmapPatch) == 32, "GeometryClipmapPatch must match the HLSL structured-buffer layout");
#endif

#endif
