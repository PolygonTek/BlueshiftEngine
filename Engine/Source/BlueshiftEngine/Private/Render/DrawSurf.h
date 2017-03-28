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
        AmbientVisible  = BIT(0),           ///< means visible surface (can be invisible for shadow caster surface)
        ShowWires       = BIT(1)            ///< means to draw wireframes
    };

    void                MakeSortKey(int entityIdx, const Material *material);

    uint32_t            sortKey;
    uint32_t            flags;
    const viewEntity_t *space;              ///< entity of this surface
    const Material *    material;           ///< material of this surface
    const float *       materialRegisters;
    SubMesh *           subMesh;            ///< sub mesh of this surface
    GuiSubMesh *        guiSubMesh;
};

// sortKey:
// 0xF0000000 (0~15)    : material sort
// 0x0FFF0000 (0~4095)  : entity index
// 0x0000FFFF (0~65535) : material index
BE_INLINE void DrawSurf::MakeSortKey(int entityIdx, const Material *material) {
    sortKey = ((material->GetSort() << 28) | (entityIdx << 16) | materialManager.GetIndexByMaterial(material));
}

BE_NAMESPACE_END
