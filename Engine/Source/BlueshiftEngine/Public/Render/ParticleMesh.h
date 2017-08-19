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

#pragma once

#include "Containers/Array.h"
#include "Core/Vertex.h"
#include "BufferCache.h"
#include "ParticleSystem.h"

BE_NAMESPACE_BEGIN

class Material;
class SceneEntity;
class SceneView;

struct PrtMeshSurf {
    const Material *        material;

    int                     numVerts;
    int                     numIndexes;
    
    BufferCache             vertexCache;
    BufferCache             indexCache;
};

class ParticleMesh {
    friend class RenderWorld;

public:
    ParticleMesh();

    int                     NumSurfaces() const { return surfaces.Count(); }
    const PrtMeshSurf &     Surface(int surfaceIndex) const { return surfaces[surfaceIndex]; }

    void                    Clear();

    void                    Draw(const ParticleSystem *particleSystem, const Array<Particle *> &stageParticles, const SceneEntity *entity, const SceneView *view);

    void                    CacheIndexes();

private:
    void                    PrepareNextSurf();
    void                    DrawQuad(const VertexGeneric *verts, const Material *material);
    int                     CountDrawingVerts(const ParticleSystem::Stage &stage, const Particle *stageParticles) const;
    void                    ComputeTextureCoordinates(const ParticleSystem::StandardModule &standardModule, float time, float &s1, float &t1, float &s2, float &t2) const;

    Array<PrtMeshSurf>      surfaces;
    PrtMeshSurf *           currentSurf;

    int                     totalVerts;         ///< Total number of the vertices
    int                     totalIndexes;       ///< Total number of the indices
};

BE_NAMESPACE_END
