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

void MeshCombiner::CombineRoot(Entity *staticRoot) {
    // Skip if this entity has mesh component that is already combined with others
    ComStaticMeshRenderer *meshRenderer = staticRoot->GetComponent<ComStaticMeshRenderer>();
    if (meshRenderer) {
        if (meshRenderer->staticBatchIndex >= 0) {
            return;
        }
    }

    Array<Entity *> staticEntities;
    staticEntities.Append(staticRoot);
    EnumerateStaticChildren(staticRoot, staticEntities);

    if (staticEntities.Count() <= 1) {
        return;
    }

    // Sort entities by material index
    staticEntities.Sort([](const Entity *e1, const Entity *e2) -> bool {
        const auto *renderable1 = e1->GetComponent<ComRenderable>();
        const auto *renderable2 = e2->GetComponent<ComRenderable>();

        if (renderable1 && renderable2) {
            const Array<Material *> &materials1 = renderable1->renderObjectDef.materials;
            const Array<Material *> &materials2 = renderable2->renderObjectDef.materials;

            if (materials1.Count() == 1 && materials2.Count() == 1) {
                const Material *material1 = materials1[0];
                const Material *material2 = materials2[0];

                return materialManager.GetIndexByMaterial(material1) > materialManager.GetIndexByMaterial(material2);
            }
        }
        return false;
    });

    Entity *combineRoot = nullptr;
    Array<ComStaticMeshRenderer *> meshRenderers;
    const Material *prevMaterial = nullptr;
    const int maxCombinedVerts = 65535;
    int numCombinedVerts = 0;
    int batchIndex = 0;

    for (int entityIndex = 0; entityIndex < staticEntities.Count(); entityIndex++) {
        Entity *entity = staticEntities[entityIndex];

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

        int numVerts = meshRenderer->referenceMesh->GetSurface(0)->subMesh->NumVerts();
        const Material *material = meshRenderer->renderObjectDef.materials[0];

        if (!combineRoot) {
            combineRoot = entity;
        }

        if (prevMaterial != material || numCombinedVerts + numVerts >= maxCombinedVerts) {
            if (meshRenderers.Count() > 1) {
                MakeCombinedMesh(combineRoot, meshRenderers, batchIndex++);
            }

            combineRoot = nullptr;

            meshRenderers.Clear();

            numCombinedVerts = 0;
        }

        meshRenderers.Append(meshRenderer);

        prevMaterial = material;
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
        combinedMeshName += " " + (batchIndex + 1);
    }

    staticBatch->SetMesh(meshManager.CreateCombinedMesh(combinedMeshName, subMeshes, localMatrices));
}

void MeshCombiner::EnumerateStaticChildren(const Entity *parentEntity, Array<Entity *> &staticChildren) {
    for (Entity *entity = parentEntity->GetNode().GetChild(); entity; entity = entity->GetNode().GetNextSibling()) {
        if (!entity->IsStatic()) {
            continue;
        }

        staticChildren.Append(entity);

        EnumerateStaticChildren(entity, staticChildren);
    }
}

BE_NAMESPACE_END
