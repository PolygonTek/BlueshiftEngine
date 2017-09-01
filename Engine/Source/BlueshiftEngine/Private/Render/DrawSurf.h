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

class DrawSurf {
public:
    enum Flag {
        AmbientVisible      = BIT(0),           ///< means visible surface (can be invisible for shadow caster surface)
        ShowWires           = BIT(1)            ///< means to draw wireframes
    };

    void                    MakeSortKey(int entityIdx, const Material *material);

    uint64_t                sortKey;
    uint32_t                flags;
    const viewEntity_t *    space;              ///< entity of this surface
    const Material *        material;           ///< material of this surface
    const float *           materialRegisters;
    SubMesh *               subMesh;
};

//---------------------------------------------------
// sortKey bits:
// 0x0000FFFF00000000 (0~65535) : material sort
// 0x00000000FFFF0000 (0~65535) : entity index
// 0x000000000000FFFF (0~65535) : material index
//---------------------------------------------------
BE_INLINE void DrawSurf::MakeSortKey(int entityIdx, const Material *material) {
    sortKey = (((uint64_t)material->GetSort() << 32) | ((uint64_t)entityIdx << 16) | (uint64_t)materialManager.GetIndexByMaterial(material));
}

BE_NAMESPACE_END
