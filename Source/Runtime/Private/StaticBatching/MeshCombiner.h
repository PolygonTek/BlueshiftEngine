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

class ComStaticMeshRenderer;
class Entity;

class MeshCombiner {
public:
    static void CombineRoot(const Hierarchy<Entity> &staticRoot);

private:
    static void EnumerateCombinableEntities(const Hierarchy<Entity> &parentNode, Array<Entity *> &staticChildren);
    static void MakeCombinedMesh(Entity *staticBatchRoot, Array<ComStaticMeshRenderer *> &meshRenderers, int batchIndex);
};

BE_NAMESPACE_END
