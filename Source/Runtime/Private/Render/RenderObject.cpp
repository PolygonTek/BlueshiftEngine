// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderUtils.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

RenderObject::RenderObject(RenderWorld *renderWorld, int index) {
    this->renderWorld = renderWorld;
    this->index = index;
}

RenderObject::~RenderObject() {
    if (proxy) {
        Mem_Free(proxy);
    }
    if (meshSurfProxies) {
        Mem_Free(meshSurfProxies);
    }
}

void RenderObject::Update(const State *stateDef) {
    state = *stateDef;

    // Saturates object color components in range [0, 1].
    Clamp(state.materialParms[MaterialParm::Red], 0.0f, 1.0f);
    Clamp(state.materialParms[MaterialParm::Green], 0.0f, 1.0f);
    Clamp(state.materialParms[MaterialParm::Blue], 0.0f, 1.0f);
    Clamp(state.materialParms[MaterialParm::Alpha], 0.0f, 1.0f);

    maxVisDistSquared = state.maxVisDist * state.maxVisDist;
    
    prevWorldMatrix = worldMatrix;

    worldMatrix = state.worldMatrix;
    worldMatrixInverse = worldMatrix.Inverse();

    worldAABB.SetFromTransformedAABBFast(state.aabb, worldMatrix);
    worldOBB = OBB(state.aabb, worldMatrix);
}

BE_NAMESPACE_END
