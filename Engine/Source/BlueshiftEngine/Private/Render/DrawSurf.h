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
