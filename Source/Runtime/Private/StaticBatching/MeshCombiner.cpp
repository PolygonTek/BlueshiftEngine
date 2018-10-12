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
#include "Game/Entity.h"
#include "Components/ComTransform.h"
#include "Components/ComStaticMeshRenderer.h"
#include "MeshCombiner.h"
#include "StaticBatching/StaticBatch.h"

BE_NAMESPACE_BEGIN

static int CompareMesh(const RenderObject::State *renderObjectDef1, const RenderObject::State *renderObjectDef2) {
    int compare = renderObjectDef1->flags - renderObjectDef2->flags;
    if (compare == 0) {
        int materialIndex1 = -1;
        int materialIndex2 = -1;

        if (renderObjectDef1->materials.Count() == 1) {
            materialIndex1 = materialManager.GetIndexByMaterial(renderObjectDef1->materials[0]);
        }
        if (renderObjectDef2->materials.Count() == 1) {
            materialIndex2 = materialManager.GetIndexByMaterial(renderObjectDef2->materials[0]);
        }

        compare = materialIndex1 - materialIndex2;
        if (compare == 0) {
            for (int i = 0; i < RenderObject::MaxMaterialParms; i++) {
                compare = *reinterpret_cast<const int32_t *>(&renderObjectDef1->materialParms[i]) - *reinterpret_cast<const int32_t *>(&renderObjectDef2->materialParms[i]);
                if (compare != 0) {
                    break;
                }
            }
        }
    }
    return compare;
}

void MeshCombiner::CombineRoot(const Hierarchy<Entity> &staticRoot) {
    Entity *staticRootEntity = staticRoot.Owner();
    if (staticRootEntity) {
        // Skip if this entity has mesh component that is already combined with others
        ComStaticMeshRenderer *meshRenderer = staticRootEntity->GetComponent<ComStaticMeshRenderer>();
        if (meshRenderer) {
            if (meshRenderer->staticBatchIndex >= 0) {
                return;
            }
        }
    }

    // Enumerate combinable entities
    Array<Entity *> combinableEntities;
    EnumerateCombinableEntities(staticRoot, combinableEntities);

    if (combinableEntities.Count() <= 1) {
        return;
    }

    // Sort entities by material
    combinableEntities.Sort([](Entity *e1, Entity *e2) -> bool {
        const auto *renderable1 = e1->GetComponent<ComRenderable>();
        const auto *renderable2 = e2->GetComponent<ComRenderable>();

        int compare = 0;

        if (renderable1 && renderable2) {
            compare = CompareMesh(&renderable1->renderObjectDef, &renderable2->renderObjectDef);
        }
        
        return compare < 0;
    });

    Entity *combineRoot = nullptr;
    Array<ComStaticMeshRenderer *> meshRenderers;
    const RenderObject::State *prevRenderObjectDef = nullptr;
    const int maxCombinedVerts = 65535;
    int numCombinedVerts = 0;
    int batchIndex = 0;

    for (int entityIndex = 0; entityIndex < combinableEntities.Count(); entityIndex++) {
        Entity *entity = combinableEntities[entityIndex];

        ComStaticMeshRenderer *meshRenderer = entity->GetComponent<ComStaticMeshRenderer>();

        int numVerts = meshRenderer->referenceMesh->GetSurface(0)->subMesh->NumVerts();

        const RenderObject::State *renderObjectDef = &meshRenderer->renderObjectDef;

        if (!combineRoot) {
            combineRoot = entity;
        }

        if ((prevRenderObjectDef && CompareMesh(prevRenderObjectDef, renderObjectDef) != 0) || numCombinedVerts + numVerts >= maxCombinedVerts) {
            if (meshRenderers.Count() > 1) {
                MakeCombinedMesh(combineRoot, meshRenderers, batchIndex++);
            }

            combineRoot = nullptr;

            meshRenderers.Clear();

            numCombinedVerts = 0;
        }

        meshRenderers.Append(meshRenderer);

        prevRenderObjectDef = renderObjectDef;
        numCombinedVerts += numVerts;
    }

    if (meshRenderers.Count() > 1) {
        MakeCombinedMesh(combineRoot, meshRenderers, batchIndex);
    }
}

void MeshCombiner::MakeCombinedMesh(Entity *staticBatchRoot, Array<ComStaticMeshRenderer *> &meshRenderers, int batchIndex) {
    assert(meshRenderers.Count() > 1);

    Mat3x4 worldToLocalMatrix = staticBatchRoot->GetTransform()->GetMatrix().Inverse();
    Array<Mat3x4> localMatrices;
    Array<SubMesh *> subMeshes;

    StaticBatch *staticBatch = StaticBatch::AllocStaticBatch(staticBatchRoot);

    for (int i = 0; i < meshRenderers.Count(); i++) {
        meshRenderers[i]->staticBatchIndex = staticBatch->GetIndex();

        Mat3x4 localMatrix = worldToLocalMatrix * meshRenderers[i]->GetEntity()->GetTransform()->GetMatrix();
        localMatrices.Append(localMatrix);

        subMeshes.Append(meshRenderers[i]->referenceMesh->GetSurface(0)->subMesh);
    }

    Str combinedMeshName = "Combined Mesh (" + staticBatchRoot->GetName() + ")";
    if (batchIndex > 0) {
        combinedMeshName += " " + Str(batchIndex + 1);
    }

    Mesh *combinedMesh = meshManager.CreateCombinedMesh(combinedMeshName, subMeshes, localMatrices);
    staticBatch->SetMesh(combinedMesh);
}

void MeshCombiner::EnumerateCombinableEntities(const Hierarchy<Entity> &rootNode, Array<Entity *> &staticChildren) {
    for (Entity *entity = rootNode.GetChild(); entity; entity = entity->GetNode().GetNext()) {
        if (!entity->IsStatic()) {
            continue;
        }

        ComStaticMeshRenderer *meshRenderer = entity->GetComponent<ComStaticMeshRenderer>();
        if (!meshRenderer || !meshRenderer->referenceMesh) {
            continue;
        }

        if (meshRenderer->referenceMesh->NumSurfaces() > 1) {
            continue;
        }

        if (meshRenderer->renderObjectDef.materials.Count() > 1) {
            continue;
        }

        staticChildren.Append(entity);
    }
}

BE_NAMESPACE_END
