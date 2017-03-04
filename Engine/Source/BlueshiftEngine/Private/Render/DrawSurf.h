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

/*
-------------------------------------------------------------------------------

    DrawSurf

    sort key:

    0xF0000000 (0~15)    : material sort
    0x0FFF0000 (0~4095)  : entityNum
    0x0000FFFF (0~65535) : materialNum

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class DrawSurf {
public:
    enum Flag {
        AmbientVisible  = BIT(0),
        ShowWires       = BIT(1)
    };

    void                MakeSortKey(int entityNum, const Material *material);

    uint32_t            sortkey;
    uint32_t            flags;
    viewEntity_t *      entity;
    const Material *    material;
    const float *       materialRegisters;
    SubMesh *           subMesh;
    GuiSubMesh *        guiSubMesh;
};

BE_INLINE void DrawSurf::MakeSortKey(int entityNum, const Material *material) {
    sortkey = ((material->GetSort() << 28) | (entityNum << 16) | materialManager.GetIndexByMaterial(material));
}

BE_NAMESPACE_END
