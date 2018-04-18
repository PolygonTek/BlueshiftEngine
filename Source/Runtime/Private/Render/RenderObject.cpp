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

RenderObject::RenderObject() {
    memset(&state, 0, sizeof(state));
    index = 0;

    worldOBB.SetZero();

    worldMatrix.SetIdentity();

    motionBlurModelMatrix[0].SetIdentity();
    motionBlurModelMatrix[1].SetIdentity();

    viewCount = 0;
    visibleObject = nullptr;
    proxy = nullptr;
    meshSurfProxies = nullptr;
    numMeshSurfProxies = 0;

    firstUpdate = true;
}

RenderObject::~RenderObject() {
    if (proxy) {
        Mem_Free(proxy);
    }
    if (meshSurfProxies) {
        Mem_Free(meshSurfProxies);
    }
}

void RenderObject::Update(const State *stateCopy) {
    state = *stateCopy;

    // Saturate object color RGBA in range [0, 1]
    Clamp(state.materialParms[RedParm], 0.0f, 1.0f);
    Clamp(state.materialParms[GreenParm], 0.0f, 1.0f);
    Clamp(state.materialParms[BlueParm], 0.0f, 1.0f);
    Clamp(state.materialParms[AlphaParm], 0.0f, 1.0f);

    if (state.joints || state.mesh) {
        worldOBB = OBB(GetAABB(), state.origin, state.axis);
    }

    // Calculate world matrix
    worldMatrix.SetLinearTransform(state.axis, state.scale, state.origin);

    if (firstUpdate) {
        motionBlurModelMatrix[1] = worldMatrix;
    } else {
        motionBlurModelMatrix[1] = motionBlurModelMatrix[0];
    }
    motionBlurModelMatrix[0] = worldMatrix;

    firstUpdate = false;
}

const AABB RenderObject::GetAABB() const {
    if (state.joints) {
        return state.aabb * state.scale;
    }

    assert(state.mesh);
    return state.mesh->GetAABB() * state.scale;
}

BE_NAMESPACE_END
