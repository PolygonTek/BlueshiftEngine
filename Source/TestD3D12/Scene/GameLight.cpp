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
#include "Scene/GameLight.h"
#include "Render/Frontend/RenderWorld.h"

bool GameLight::SetEnabled(RenderWorld *renderWorld, bool enabled) {
    assert(renderWorld);

    if (!enabled) {
        if (renderLightHandle >= 0) {
            renderWorld->RemoveRenderLight(renderLightHandle);
            renderLightHandle = -1;
        }
        return false;
    }

    if (renderLightHandle >= 0) {
        return true;
    }

    RenderLight::Desc desc;
    desc.type = type;
    desc.flags = flags;
    desc.layer = layer;
    desc.origin = origin;
    desc.axis = axis;
    desc.range = range;
    desc.innerConeAngle = innerConeAngle;
    desc.outerConeAngle = outerConeAngle;
    desc.zNear = zNear;
    desc.linearColor = linearColor;
    desc.intensity = intensity;
    desc.shadowOffsetUnits = shadowOffsetUnits;
    desc.shadowOffsetFactor = shadowOffsetFactor;

    renderLightHandle = renderWorld->AddRenderLight(desc);
    return renderLightHandle >= 0;
}
