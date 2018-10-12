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
#include "RenderInternal.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

static AABB mergeAABB;

using MeshSurfPtr = MeshSurf *;
template <typename MeshSurfPtr>
static bool ListSortCompareMeshMaterials(const MeshSurfPtr &a, const MeshSurfPtr &b) {
    int index_a = (a)->materialIndex;
    int index_b = (b)->materialIndex;

    return index_a < index_b;
}

using MeshSurfPtr = MeshSurf *;
template <typename MeshSurfPtr>
static bool ListSortCompareMergedAABBArea(const MeshSurfPtr &a, const MeshSurfPtr &b) {
    const AABB &aAABB = (a)->subMesh->GetAABB();
    const AABB &bAABB = (b)->subMesh->GetAABB();

    float aArea = (mergeAABB + aAABB).Area();
    float bArea = (mergeAABB + bAABB).Area();

    if (aArea == bArea) {
        float aDist = aAABB.DistanceSqr(mergeAABB.Center());
        float bDist = bAABB.DistanceSqr(mergeAABB.Center());

        return aDist < bDist;
    }

    return aArea < bArea;
}

void Mesh::SortAndMerge() {
    int     numNewVerts;
    int     numNewIndexes;

    ComputeAABB();

    if (numJoints > 1) {
        return;
    }

    if (surfaces.Count() <= 1) {
        return;
    }

    surfaces.Sort(ListSortCompareMeshMaterials<MeshSurfPtr>);

    int m0 = 0;
    int m1 = 0;

    int numNewSurfs = 0;

    while (m1 < surfaces.Count()) {
        if (m1 + 1 < surfaces.Count()) {
            numNewVerts = 0;
            numNewIndexes = 0;

            int m2 = m0;
            while (m2 + 1 < surfaces.Count() && 
                surfaces[m0]->materialIndex == surfaces[m2 + 1]->materialIndex) {
                m2++;
            }

            if (m2 > m0) {
                // m0 의 AABB 와 합쳤을 때 area 가 작은 순으로 sorting
                mergeAABB = surfaces[m0]->subMesh->aabb;
                surfaces.SortSubSection(m0, m2, ListSortCompareMergedAABBArea<MeshSurfPtr>);
            }

            // 합칠 기준 AABB 를 확장
            AABB aabb1 = surfaces[m0]->subMesh->aabb;
            aabb1.ExpandSelf(MeterToUnit(10));

            while (m1 + 1 < surfaces.Count() && 
                surfaces[m0]->materialIndex == surfaces[m1 + 1]->materialIndex && 
                numNewIndexes < 32768) {
                AABB aabb2 = surfaces[m1 + 1]->subMesh->aabb;

                if (!aabb1.IsIntersectAABB(aabb2)) {
                    break;
                }

                // 합쳐질 AABB 의 면적이 80% 이하일 경우 중단
                AABB aabb3 = aabb1.Intersect(aabb2);
                if (aabb3.Area() / aabb2.Area() <= 0.8f && numNewIndexes > 36) {
                    break;
                }

                numNewVerts += surfaces[m1 + 1]->subMesh->numVerts;
                numNewIndexes += surfaces[m1 + 1]->subMesh->numIndexes;

                m1++;
            }
        }

        if (m1 - m0 > 0) {
            numNewVerts += surfaces[m0]->subMesh->numVerts;
            numNewIndexes += surfaces[m0]->subMesh->numIndexes;

            MeshSurf *newSurf = AllocSurface(numNewVerts, numNewIndexes);
            newSurf->materialIndex = surfaces[m0]->materialIndex;

            VertexGenericLit *vp = newSurf->subMesh->verts;
            TriIndex *ip = newSurf->subMesh->indexes;

            int filledVertexCount = 0;

            for (int i = m0; i <= m1; i++) {
                MeshSurf *surf = surfaces[i];

                simdProcessor->Memcpy(vp, surf->subMesh->verts, sizeof(VertexGenericLit) * surf->subMesh->numVerts);
                vp += surf->subMesh->numVerts;

                for (int j = 0; j < surf->subMesh->numIndexes; j++) {
                    ip[j] = surf->subMesh->indexes[j] + filledVertexCount;
                }
                ip += surf->subMesh->numIndexes;

                filledVertexCount += surf->subMesh->numVerts;

                FreeSurface(surf);
                surfaces[i] = nullptr;
            }

            surfaces[numNewSurfs] = newSurf;
            numNewSurfs++;

            m0 = m1;
        } else {
            surfaces[numNewSurfs] = surfaces[m0];
            numNewSurfs++;
        }

        m0++;
        m1++;
    }

    surfaces.Resize(numNewSurfs);
}

BE_NAMESPACE_END
