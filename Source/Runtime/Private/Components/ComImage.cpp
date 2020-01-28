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
#include "Core/Heap.h"
#include "Components/ComImage.h"
#include "Components/ComRectTransform.h"
#include "Game/Entity.h"
#include "Render/Render.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Image", ComImage, ComRenderable)
BEGIN_EVENTS(ComImage)
END_EVENTS

void ComImage::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("imageMaterial", "Image Material", Guid, GetMaterialGuid, SetMaterialGuid, GuidMapper::defaultMaterialGuid,
        "", PropertyInfo::Flag::Editor).SetMetaObject(&MaterialResource::metaObject);
    REGISTER_ACCESSOR_PROPERTY("imageType", "Image Type", ImageType::Enum, GetImageType, SetImageType, ImageType::Simple,
        "", PropertyInfo::Flag::Editor).SetEnumString("Simple;Sliced");
}

ComImage::ComImage() {
    renderObjectDef.materials.SetCount(1);
    renderObjectDef.materials[0] = nullptr;
}

ComImage::~ComImage() {
    Purge(false);
}

void ComImage::Purge(bool chainPurge) {
    if (renderObjectDef.verts) {
        Mem_AlignedFree(renderObjectDef.verts);
        renderObjectDef.verts = nullptr;
    }

    if (renderObjectDef.indexes) {
        Mem_AlignedFree(renderObjectDef.indexes);
        renderObjectDef.indexes = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComImage::Init() {
    ComRenderable::Init();

    // Don't account maxVisDist when rendering this render object.
    renderObjectDef.flags |= RenderObject::Flag::NoVisDist;

    ComRectTransform *rectTransform = GetEntity()->GetRectTransform();

    renderObjectDef.aabb = rectTransform->GetAABB();

    rectTransform->Connect(&ComRectTransform::SIG_RectTransformUpdated, this, (SignalCallback)&ComImage::RectTransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

Guid ComImage::GetMaterialGuid() const {
    if (renderObjectDef.materials[0]) {
        const Str materialPath = renderObjectDef.materials[0]->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void ComImage::SetMaterialGuid(const Guid &materialGuid) {
    // Release the previously used material
    if (renderObjectDef.materials[0]) {
        materialManager.ReleaseMaterial(renderObjectDef.materials[0]);
    }

    // Get the new material
    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    renderObjectDef.materials[0] = materialManager.GetMaterial(materialPath);

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

Material *ComImage::GetMaterial() const {
    Guid materialGuid = GetMaterialGuid();
    if (materialGuid.IsZero()) {
        return nullptr;
    }

    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    return materialManager.GetMaterial(materialPath); // FIXME: release ?
}

void ComImage::SetMaterial(const Material *material) {
    const Guid materialGuid = resourceGuidMapper.Get(material->GetHashName());

    SetMaterialGuid(materialGuid);
}

ComImage::ImageType::Enum ComImage::GetImageType() const {
    return imageType;
}

void ComImage::SetImageType(ImageType::Enum imageType) {
    this->imageType = imageType;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComImage::RectTransformUpdated(const ComRectTransform *rectTransform) {
    renderObjectDef.aabb = rectTransform->GetAABB();

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComImage::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    UpdateRawVertexes();

    ComRenderable::UpdateVisuals();
}

void ComImage::UpdateRawVertexes() {
    const ComRectTransform *rectTransform = GetEntity()->GetRectTransform();
    const RectF rect = rectTransform->GetLocalRect();

    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.X2();
    float y2 = rect.Y2();

    if (imageType == ImageType::Simple) {
        if (renderObjectDef.numVerts != 4 && renderObjectDef.numIndexes != 6) {
            if (renderObjectDef.verts) {
                Mem_AlignedFree(renderObjectDef.verts);
            }
            renderObjectDef.verts = (VertexGeneric *)Mem_Alloc16(sizeof(renderObjectDef.verts[0]) * 4);

            if (renderObjectDef.indexes) {
                Mem_AlignedFree(renderObjectDef.indexes);
            }
            renderObjectDef.indexes = (TriIndex *)Mem_Alloc16(sizeof(renderObjectDef.indexes[0]) * 6);
        }

        renderObjectDef.numVerts = 4;

        renderObjectDef.verts[0].SetPosition(x1, y1, 0);
        renderObjectDef.verts[0].SetTexCoord(0, 1);
        renderObjectDef.verts[0].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[1].SetPosition(x2, y1, 0);
        renderObjectDef.verts[1].SetTexCoord(1, 1);
        renderObjectDef.verts[1].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[2].SetPosition(x2, y2, 0);
        renderObjectDef.verts[2].SetTexCoord(1, 0);
        renderObjectDef.verts[2].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[3].SetPosition(x1, y2, 0);
        renderObjectDef.verts[3].SetTexCoord(0, 0);
        renderObjectDef.verts[3].SetColor(0xFFFFFFFF);

        renderObjectDef.numIndexes = 6;

        renderObjectDef.indexes[0] = 0;
        renderObjectDef.indexes[1] = 1;
        renderObjectDef.indexes[2] = 2;
        renderObjectDef.indexes[3] = 0;
        renderObjectDef.indexes[4] = 2;
        renderObjectDef.indexes[5] = 3;
    } else if (imageType == ImageType::Sliced) {
        if (renderObjectDef.numVerts != 16 && renderObjectDef.numIndexes != 54) {
            if (renderObjectDef.verts) {
                Mem_AlignedFree(renderObjectDef.verts);
            }
            renderObjectDef.verts = (VertexGeneric *)Mem_Alloc16(sizeof(renderObjectDef.verts[0]) * 16);

            if (renderObjectDef.indexes) {
                Mem_AlignedFree(renderObjectDef.indexes);
            }
            renderObjectDef.indexes = (TriIndex *)Mem_Alloc16(sizeof(renderObjectDef.indexes[0]) * 54);
        }

        // HACK
        const Texture *imageTexture = renderObjectDef.materials[0]->GetPass()->shaderProperties.GetByIndex(0)->second.texture;

        float w = imageTexture->GetWidth();
        float h = imageTexture->GetHeight();

        const Point &borderLT = renderObjectDef.materials[0]->GetImageBorderLT();
        const Point &borderRB = renderObjectDef.materials[0]->GetImageBorderRB();

        float bx = borderLT.x + borderRB.x;
        float by = borderLT.y + borderRB.y;

        float x1b = x1 + borderLT.x;
        float x2b = x2 - borderRB.x;
        float y1b = y1 + borderLT.y;
        float y2b = y2 - borderRB.y;

        if (bx > x2 - x1) {
            x1b = Math::Lerp(x1, x2, borderLT.x / bx);
            x2b = x1b;
        }

        if (by > y2 - y1) {
            y1b = Math::Lerp(y1, y2, borderLT.y / by);
            y2b = y1b;
        }

        float s1b = borderLT.x / w;
        float s2b = 1.0f - borderRB.x / w;
        float t1b = borderLT.y / h;
        float t2b = 1.0f - borderRB.y / w;

        renderObjectDef.numVerts = 16;

        renderObjectDef.verts[0].SetPosition(x1, y1, 0);
        renderObjectDef.verts[0].SetTexCoord(0, 1);
        renderObjectDef.verts[0].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[1].SetPosition(x1b, y1, 0);
        renderObjectDef.verts[1].SetTexCoord(s1b, 1);
        renderObjectDef.verts[1].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[2].SetPosition(x2b, y1, 0);
        renderObjectDef.verts[2].SetTexCoord(s2b, 1);
        renderObjectDef.verts[2].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[3].SetPosition(x2, y1, 0);
        renderObjectDef.verts[3].SetTexCoord(1, 1);
        renderObjectDef.verts[3].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[4].SetPosition(x1, y1b, 0);
        renderObjectDef.verts[4].SetTexCoord(0, t2b);
        renderObjectDef.verts[4].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[5].SetPosition(x1b, y1b, 0);
        renderObjectDef.verts[5].SetTexCoord(s1b, t2b);
        renderObjectDef.verts[5].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[6].SetPosition(x2b, y1b, 0);
        renderObjectDef.verts[6].SetTexCoord(s2b, t2b);
        renderObjectDef.verts[6].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[7].SetPosition(x2, y1b, 0);
        renderObjectDef.verts[7].SetTexCoord(1, t2b);
        renderObjectDef.verts[7].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[8].SetPosition(x1, y2b, 0);
        renderObjectDef.verts[8].SetTexCoord(0, t1b);
        renderObjectDef.verts[8].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[9].SetPosition(x1b, y2b, 0);
        renderObjectDef.verts[9].SetTexCoord(s1b, t1b);
        renderObjectDef.verts[9].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[10].SetPosition(x2b, y2b, 0);
        renderObjectDef.verts[10].SetTexCoord(s2b, t1b);
        renderObjectDef.verts[10].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[11].SetPosition(x2, y2b, 0);
        renderObjectDef.verts[11].SetTexCoord(1, t1b);
        renderObjectDef.verts[11].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[12].SetPosition(x1, y2, 0);
        renderObjectDef.verts[12].SetTexCoord(0, 0);
        renderObjectDef.verts[12].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[13].SetPosition(x1b, y2, 0);
        renderObjectDef.verts[13].SetTexCoord(s1b, 0);
        renderObjectDef.verts[13].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[14].SetPosition(x2b, y2, 0);
        renderObjectDef.verts[14].SetTexCoord(s2b, 0);
        renderObjectDef.verts[14].SetColor(0xFFFFFFFF);

        renderObjectDef.verts[15].SetPosition(x2, y2, 0);
        renderObjectDef.verts[15].SetTexCoord(1, 0);
        renderObjectDef.verts[15].SetColor(0xFFFFFFFF);

        renderObjectDef.numIndexes = 54;

        renderObjectDef.indexes[0] = 0;
        renderObjectDef.indexes[1] = 1;
        renderObjectDef.indexes[2] = 5;
        renderObjectDef.indexes[3] = 0;
        renderObjectDef.indexes[4] = 5;
        renderObjectDef.indexes[5] = 4;
        renderObjectDef.indexes[6] = 1;
        renderObjectDef.indexes[7] = 2;
        renderObjectDef.indexes[8] = 6;
        renderObjectDef.indexes[9] = 1;
        renderObjectDef.indexes[10] = 6;
        renderObjectDef.indexes[11] = 5;
        renderObjectDef.indexes[12] = 2;
        renderObjectDef.indexes[13] = 3;
        renderObjectDef.indexes[14] = 7;
        renderObjectDef.indexes[15] = 2;
        renderObjectDef.indexes[16] = 7;
        renderObjectDef.indexes[17] = 6;
        renderObjectDef.indexes[18] = 4;
        renderObjectDef.indexes[19] = 5;
        renderObjectDef.indexes[20] = 9;
        renderObjectDef.indexes[21] = 4;
        renderObjectDef.indexes[22] = 9;
        renderObjectDef.indexes[23] = 8;
        renderObjectDef.indexes[24] = 5;
        renderObjectDef.indexes[25] = 6;
        renderObjectDef.indexes[26] = 10;
        renderObjectDef.indexes[27] = 5;
        renderObjectDef.indexes[28] = 10;
        renderObjectDef.indexes[29] = 9;
        renderObjectDef.indexes[30] = 6;
        renderObjectDef.indexes[31] = 7;
        renderObjectDef.indexes[32] = 11;
        renderObjectDef.indexes[33] = 6;
        renderObjectDef.indexes[34] = 11;
        renderObjectDef.indexes[35] = 10;
        renderObjectDef.indexes[36] = 8;
        renderObjectDef.indexes[37] = 9;
        renderObjectDef.indexes[38] = 13;
        renderObjectDef.indexes[39] = 8;
        renderObjectDef.indexes[40] = 13;
        renderObjectDef.indexes[41] = 12;
        renderObjectDef.indexes[42] = 9;
        renderObjectDef.indexes[43] = 10;
        renderObjectDef.indexes[44] = 14;
        renderObjectDef.indexes[45] = 9;
        renderObjectDef.indexes[46] = 14;
        renderObjectDef.indexes[47] = 13;
        renderObjectDef.indexes[48] = 10;
        renderObjectDef.indexes[49] = 11;
        renderObjectDef.indexes[50] = 15;
        renderObjectDef.indexes[51] = 10;
        renderObjectDef.indexes[52] = 15;
        renderObjectDef.indexes[53] = 14;
    }
}

BE_NAMESPACE_END
