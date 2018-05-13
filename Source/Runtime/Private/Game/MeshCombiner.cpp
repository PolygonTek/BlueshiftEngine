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
#include "Components/ComRenderable.h"
#include "Components/ComMeshRenderer.h"
#include "MeshCombiner.h"

BE_NAMESPACE_BEGIN

void MeshCombiner::EnumerateStaticChildren(const Entity *parent, Array<Entity *> &staticChildren) {
    for (Entity *ent = parent->GetNode().GetChild(); ent; ent = ent->GetNode().GetNextSibling()) {
        if (!ent->IsStatic()) {
            continue;
        }

        const auto *meshRenderer = ent->GetComponent<ComMeshRenderer>();
        if (!meshRenderer || !meshRenderer->referenceMesh) {
            continue;
        }

        if (meshRenderer->referenceMesh->NumSurfaces() > 1) {
            continue;
        }

        if (meshRenderer->renderObjectDef.materials.Count() > 1) {
            continue;
        }

        staticChildren.Append(ent);

        EnumerateStaticChildren(ent, staticChildren);
    }
}

void MeshCombiner::CombineAll(Hierarchy<Entity> &entityHierarchy) {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->GetNode().GetNext()) {
        CombineRoot(ent);
    }
}

void MeshCombiner::CombineRoot(Entity *staticRoot) {
    const auto *meshRenderer = staticRoot->GetComponent<ComMeshRenderer>();
    if (meshRenderer->staticBatchIndex >= 0) {
        return;
    }

    Array<Entity *> staticChildren;
    EnumerateStaticChildren(staticRoot, staticChildren);

    if (staticChildren.Count() == 0) {
        return;
    }

    // Sort child entities by material index
    staticChildren.Sort([](const Entity *e1, const Entity *e2) -> bool {
        const auto &renderObjectDef1 = e1->GetComponent<ComRenderable>()->renderObjectDef;
        const auto &renderObjectDef2 = e2->GetComponent<ComRenderable>()->renderObjectDef;

        const Material *material1 = renderObjectDef1.materials[0];
        const Material *material2 = renderObjectDef2.materials[0];

        return materialManager.GetIndexByMaterial(material1) > materialManager.GetIndexByMaterial(material2);
    });

    Array<ComMeshRenderer *> meshRenderers;
    const Material *prevMaterial = nullptr;
    const int maxCombinedVerts = 65535;
    int numCombinedVerts = 0;

    for (int childIndex = 0; childIndex < staticChildren.Count(); childIndex++) {
        Entity *childEntity = staticChildren[childIndex];

        ComMeshRenderer *meshRenderer = childEntity->GetComponent<ComMeshRenderer>();
        int numVerts = meshRenderer->referenceMesh->GetSurface(0)->subMesh->NumVerts();
        const Material *material = meshRenderer->renderObjectDef.materials[0];

        if (prevMaterial != material || numCombinedVerts + numVerts >= maxCombinedVerts) {
            MakeCombinedMesh(staticRoot, meshRenderers);

            meshRenderers.Clear();

            numCombinedVerts = 0;
        }

        meshRenderers.Append(meshRenderer);

        prevMaterial = material;
        numCombinedVerts += numVerts;
    }

    if (meshRenderers.Count() > 1) {
        MakeCombinedMesh(staticRoot, meshRenderers);
    }
}

void MeshCombiner::MakeCombinedMesh(Entity *staticRoot, Array<ComMeshRenderer *> &meshRenderers) {
    StaticBatch *staticBatch = new StaticBatch(staticRoot);

    Mat3x4 worldToLocalMatrix = staticRoot->GetTransform()->GetMatrix().Inverse();

    Array<Mat3x4> localMatrices;
    Array<SubMesh *> subMeshes;

    for (int i = 0; i < meshRenderers.Count(); i++) {
        //meshRenderers[i]->staticBatchIndex = staticBatch->index;

        Mat3x4 localMatrix = worldToLocalMatrix * meshRenderers[i]->GetEntity()->GetTransform()->GetMatrix();
        localMatrices.Append(localMatrix);

        subMeshes.Append(meshRenderers[i]->referenceMesh->GetSurface(0)->subMesh);
    }

    Str combinedMeshName = "Combined";

    staticBatch->referenceMesh = meshManager.CreateCombinedMesh(combinedMeshName, subMeshes, localMatrices);
}

BE_NAMESPACE_END