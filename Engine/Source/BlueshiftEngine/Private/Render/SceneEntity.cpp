#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderUtils.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

SceneEntity::SceneEntity() {
    memset(&parms, 0, sizeof(parms));
    index = 0;
    firstUpdate = true;
    worldOBB.SetZero();
    modelMatrix.SetIdentity();
    motionBlurModelMatrix[0].SetIdentity();
    motionBlurModelMatrix[1].SetIdentity();
    viewCount = 0;
    viewEntity = nullptr;
    proxy = nullptr;
    meshSurfProxies = nullptr;
    numMeshSurfProxies = 0;
}

SceneEntity::~SceneEntity() {
    if (proxy) {
        Mem_Free(proxy);
    }
    if (meshSurfProxies) {
        Mem_Free(meshSurfProxies);
    }
}

void SceneEntity::Update(const Parms *entityParms) {
    parms = *entityParms;

    Clamp(parms.materialParms[RedParm], 0.0f, 1.0f);
    Clamp(parms.materialParms[GreenParm], 0.0f, 1.0f);
    Clamp(parms.materialParms[BlueParm], 0.0f, 1.0f);
    Clamp(parms.materialParms[AlphaParm], 0.0f, 1.0f);

    if (parms.joints || parms.mesh) {
        worldOBB = OBB(GetAABB(), parms.origin, parms.axis);
    }

    modelMatrix.SetLinearTransform(parms.axis, parms.scale, parms.origin);

    if (firstUpdate) {
        motionBlurModelMatrix[1] = modelMatrix;
    } else {
        motionBlurModelMatrix[1] = motionBlurModelMatrix[0];
    }
    motionBlurModelMatrix[0] = modelMatrix;

    firstUpdate = false;
}

const AABB SceneEntity::GetAABB() const {
    if (parms.joints) {
        return parms.aabb * parms.scale;
    }

    assert(parms.mesh);
    return parms.mesh->GetAABB() * parms.scale;
}

BE_NAMESPACE_END
