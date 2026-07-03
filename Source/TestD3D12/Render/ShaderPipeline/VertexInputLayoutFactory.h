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

#include "RHI/RHI.h"

enum class VertexInputLayoutId : uint8_t {
    Xyz,
    XyzSkinning1,
    XyzSkinning4,
    XyzSkinning8,
    XyzSt,
    XyzStSkinning1,
    XyzStSkinning4,
    XyzStSkinning8,
    XyzStColor,
    XyzStColorSkinning1,
    XyzStColorSkinning4,
    XyzStColorSkinning8,
    XyzStColorNormalTangent,
    XyzStColorNormalTangentSkinning1,
    XyzStColorNormalTangentSkinning4,
    XyzStColorNormalTangentSkinning8,
    Count
};

class VertexInputLayoutFactory {
public:
    VertexInputLayoutFactory() = delete;

    static void                     Init();
    static void                     Shutdown();

    static const RHI::InputLayout & Get(VertexInputLayoutId id);

    static VertexInputLayoutId      GetIdForVertexWeights(VertexInputLayoutId baseId, int maxVertexWeights);
    static bool                     IsSkinning(VertexInputLayoutId id);
};
