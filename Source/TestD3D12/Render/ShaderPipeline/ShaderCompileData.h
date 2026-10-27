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

#include "Containers/StrArray.h"
#include "RHI/RHI.h"

struct ShaderStageSource {
    RHI::ShaderModel                shaderModel = RHI::Renderer::MinimumShaderModel();
    BE1::Str                        filename;
    BE1::Str                        entryPoint;
};

struct ShaderCompileData {
    void                            Clear();

    ShaderStageSource               stageSources[to_int(RHI::ShaderStage::Count)] = {};
    BE1::StrArray                   compileDefines;
};

BE_INLINE void ShaderCompileData::Clear() {
    for (ShaderStageSource &stageSource : stageSources) {
        stageSource = ShaderStageSource();
    }
    compileDefines.Clear();
}
