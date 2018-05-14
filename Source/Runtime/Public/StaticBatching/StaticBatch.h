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

BE_NAMESPACE_BEGIN

class Entity;
class Mesh;

class StaticBatch {
public:
    StaticBatch();
    ~StaticBatch();

    static StaticBatch *        AllocStaticBatch(Entity *rootEntity);
    static void                 DestroyStaticBatch(StaticBatch *staticBatch);

    static void                 ClearAllStaticBatches();

    static void                 CombineAll(Hierarchy<Entity> &entityHierarchy);

    static StaticBatch *        GetStaticBatchByIndex(int index);

    int                         GetIndex() const { return index; }
   
    Entity *                    GetRootEntity() const { return rootEntity; }

    Mesh *                      GetMesh() const { return referenceMesh; }
    void                        SetMesh(Mesh *mesh) { referenceMesh = mesh; }

private:
    int                         index;
    Entity *                    rootEntity;
    Mesh *                      referenceMesh;

    static Array<StaticBatch *> staticBatches;
};

BE_NAMESPACE_END
