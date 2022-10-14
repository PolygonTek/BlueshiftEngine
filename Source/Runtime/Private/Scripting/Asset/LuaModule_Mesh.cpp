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
#include "Scripting/LuaVM.h"
#include "Render/Mesh.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMesh(LuaCpp::Module &module) {
    LuaCpp::Selector _Mesh = module["Mesh"];

    _Mesh.SetClass<Mesh>();
    _Mesh.AddClassMembers<Mesh>(
        "name", &Mesh::GetName,
        "aabb", &Mesh::GetAABB,
        "is_static_mesh", &Mesh::IsStaticMesh,
        "is_skinned_mesh", &Mesh::IsSkinnedMesh,
        "compute_volume", &Mesh::ComputeVolume,
        "compute_centroid", &Mesh::ComputeCentroid);

    _Mesh["new"].SetFunc([]() {
        Guid newGuid = Guid::CreateGuid();
        Str meshName = Str("Mesh-") + newGuid.ToString();
        resourceGuidMapper.Set(newGuid, meshName);
        return meshManager.AllocMesh(meshName);
    });
    _Mesh["release"].SetFunc([](Mesh *mesh) {
        meshManager.ReleaseMesh(mesh);
    });
    _Mesh["try_slice_mesh"].SetFunc([](const Mesh &srcMesh, const Plane &slicePlane, bool generateCap, float capTextureScale, bool generateOtherMesh, Mesh *outSlicedMesh, Mesh *outOtherMesh) {
        return Mesh::TrySliceMesh(srcMesh, slicePlane, generateCap, capTextureScale, generateOtherMesh, outSlicedMesh, outOtherMesh);
    });
}

BE_NAMESPACE_END
