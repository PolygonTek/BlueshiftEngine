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

BE_NAMESPACE_BEGIN

class MeshCombiner;

class StaticBatch {
    friend class MeshCombiner;

public:
    StaticBatch(Entity *rootEntity);

    Mesh *          GetMesh() const { return referenceMesh; }

private:
    Entity *        rootEntity;
    Mesh *          referenceMesh;
};

class MeshCombiner {
public:
    static void CombineAll(Hierarchy<Entity> &entityHierarchy);
    static void CombineRoot(Entity *staticRoot);

private:
    static void EnumerateStaticChildren(const Entity *parent, Array<Entity *> &staticChildren);
    static void MakeCombinedMesh(Entity *staticRoot, Array<ComMeshRenderer *> &meshRenderers);
};

BE_NAMESPACE_END