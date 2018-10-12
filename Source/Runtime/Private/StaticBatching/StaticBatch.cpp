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
#include "Containers/Hierarchy.h"
#include "Render/Render.h"
#include "Game/Entity.h"
#include "Components/ComStaticMeshRenderer.h"
#include "StaticBatching/StaticBatch.h"
#include "../StaticBatching/MeshCombiner.h"

BE_NAMESPACE_BEGIN

Array<StaticBatch *> StaticBatch::staticBatches;

StaticBatch::StaticBatch() {
    index = -1;
    rootEntity = nullptr;
    referenceMesh = nullptr;
}

StaticBatch::~StaticBatch() {
    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh, true);
    }
}

StaticBatch *StaticBatch::AllocStaticBatch(Entity *rootEntity) {
    StaticBatch *staticBatch = new StaticBatch;
    staticBatch->index = staticBatches.Append(staticBatch);
    staticBatch->rootEntity = rootEntity;

    return staticBatch;
}

void StaticBatch::DestroyStaticBatch(StaticBatch *staticBatch) {
    staticBatches.Remove(staticBatch);

    delete staticBatch;
}

void StaticBatch::CombineAll(Hierarchy<Entity> &entityHierarchy) {
    MeshCombiner::CombineRoot(entityHierarchy);

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->GetNode().GetNext()) {
        ComStaticMeshRenderer *staticMeshRenderer = ent->GetComponent<ComStaticMeshRenderer>();
        
        if (staticMeshRenderer && staticMeshRenderer->staticBatchIndex >= 0) {
            staticMeshRenderer->OnInactive();

            const StaticBatch *staticBatch = StaticBatch::GetStaticBatchByIndex(staticMeshRenderer->staticBatchIndex);

            if (staticBatch->GetRootEntity() == ent) {
                if (staticMeshRenderer->renderObjectDef.mesh) {
                    meshManager.ReleaseMesh(staticMeshRenderer->renderObjectDef.mesh);

                    staticMeshRenderer->renderObjectDef.mesh = staticBatch->GetMesh()->InstantiateMesh(Mesh::StaticMesh);
                    staticMeshRenderer->renderObjectDef.localAABB = staticBatch->GetMesh()->GetAABB();

                    staticMeshRenderer->UpdateVisuals();
                }
            }
        }
    }
}

void StaticBatch::ClearAllStaticBatches() {
    staticBatches.DeleteContents(false);
}

StaticBatch *StaticBatch::GetStaticBatchByIndex(int index) {
    if (index < 0 || index >= staticBatches.Count()) {
        return nullptr;
    }

    return staticBatches[index];
}

BE_NAMESPACE_END
