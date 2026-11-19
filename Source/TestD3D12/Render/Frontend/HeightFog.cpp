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
#include "HeightFog.h"

bool HeightFog::Desc::IsValid() const {
    return std::isfinite(baseHeight) &&
        heightFalloff >= 0.0f && heightFalloff <= 10.0f &&
        maxDistance > 0.0f && maxDistance <= 1000000.0f &&
        medium.baseExtinction >= 0.0f && medium.baseExtinction <= 1000.0f &&
        medium.scatteringAlbedo.x >= 0.0f && medium.scatteringAlbedo.x <= 1.0f &&
        medium.scatteringAlbedo.y >= 0.0f && medium.scatteringAlbedo.y <= 1.0f &&
        medium.scatteringAlbedo.z >= 0.0f && medium.scatteringAlbedo.z <= 1.0f &&
        std::isfinite(medium.baseEmission.x) && medium.baseEmission.x >= 0.0f &&
        std::isfinite(medium.baseEmission.y) && medium.baseEmission.y >= 0.0f &&
        std::isfinite(medium.baseEmission.z) && medium.baseEmission.z >= 0.0f &&
        ambientColor.r >= 0.0f && ambientColor.r <= 1.0f &&
        ambientColor.g >= 0.0f && ambientColor.g <= 1.0f &&
        ambientColor.b >= 0.0f && ambientColor.b <= 1.0f &&
        ambientIntensity >= 0.0f && ambientIntensity <= 32.0f;
}

bool HeightFog::Desc::IsActive() const {
    return enabled && (medium.baseExtinction > 0.0f || medium.baseEmission.x > 0.0f || medium.baseEmission.y > 0.0f || medium.baseEmission.z > 0.0f);
}
