// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RenderObject.h"
#include "Render/Mesh/MeshInstance.h"

static const BE1::AABB &GetEffectiveLocalAABB(const RenderObject::Desc &desc) {
    if (!desc.aabb.IsCleared()) {
        return desc.aabb;
    }
    if (desc.meshInstance && desc.meshInstance->IsSourceReady()) {
        return desc.meshInstance->GetAABB();
    }
    return desc.aabb;
}

static bool HasNonUniformScale(const BE1::Mat3x4 &worldMatrix) {
    const float scaleXSqr = worldMatrix[0][0] * worldMatrix[0][0] + worldMatrix[1][0] * worldMatrix[1][0] + worldMatrix[2][0] * worldMatrix[2][0];
    const float scaleYSqr = worldMatrix[0][1] * worldMatrix[0][1] + worldMatrix[1][1] * worldMatrix[1][1] + worldMatrix[2][1] * worldMatrix[2][1];
    const float scaleZSqr = worldMatrix[0][2] * worldMatrix[0][2] + worldMatrix[1][2] * worldMatrix[1][2] + worldMatrix[2][2] * worldMatrix[2][2];
    const float minScaleSqr = BE1::Min3(scaleXSqr, scaleYSqr, scaleZSqr);
    const float maxScaleSqr = BE1::Max3(scaleXSqr, scaleYSqr, scaleZSqr);
    return maxScaleSqr - minScaleSqr > maxScaleSqr * MATRIX_EPSILON;
}

void RenderObject::UpdateFlags(Flag flags) {
    desc.flags = flags;
}

void RenderObject::UpdateLayer(uint8_t layer) {
    desc.layer = layer;
}

void RenderObject::UpdateTransform(const BE1::Mat3x4 &worldMatrix) {
    desc.worldMatrix = worldMatrix;
    nonUniformScale = HasNonUniformScale(worldMatrix);
    UpdateWorldAABB();
}

void RenderObject::UpdateBounds(const BE1::AABB &aabb) {
    desc.aabb = aabb;
    UpdateWorldAABB();
}

void RenderObject::UpdateMesh(const SharedPtr<MeshInstance> &meshInstance) {
    desc.meshInstance = meshInstance;
    UpdateWorldAABB();
}

void RenderObject::UpdateSkeleton(const SharedPtr<Skeleton> &skeleton) {
    desc.skeleton = skeleton;
}

void RenderObject::UpdateMeshLOD(int lodLevel) {
    desc.meshLOD = static_cast<int8_t>(BE1::Clamp(lodLevel, -1, SubMesh::MaxLODLevel));
}

void RenderObject::UpdateMaterials(const BE1::Array<SharedPtr<Material>> &materials) {
    if (materials.Count() > MaxMaterials) {
        BE_WARNLOG("RenderObject::UpdateMaterials: too many materials (%i, maximum %i)\n", materials.Count(), MaxMaterials);
        return;
    }

    desc.materials = materials;
}

void RenderObject::UpdateFadeOpacity(float fadeOpacity) {
    desc.fadeOpacity = BE1::Clamp(fadeOpacity, 0.0f, 1.0f);
}

void RenderObject::UpdateFont(const SharedPtr<Font> &font) {
    desc.GetTextDesc().font = font;
}

void RenderObject::UpdateText(const BE1::Str &text) {
    desc.GetTextDesc().text = text;
}

void RenderObject::UpdateTextStyle(const TextDesc &textDesc) {
    TextDesc &dst = desc.GetTextDesc();
    dst.textRect = textDesc.textRect;
    dst.textShadowOffset = textDesc.textShadowOffset;
    dst.textShadowColor = textDesc.textShadowColor;
    dst.textParams = textDesc.textParams;
    dst.textScale = textDesc.textScale;
    dst.textLineSpacing = textDesc.textLineSpacing;
}

void RenderObject::UpdateWorldAABB() {
    const BE1::AABB &localAABB = GetEffectiveLocalAABB(desc);
    if (localAABB.IsCleared()) {
        worldAABB = BE1::AABB::empty;
    } else {
        worldAABB.SetFromTransformedAABB(localAABB, desc.worldMatrix);
    }
}

void RenderObject::Update(const RenderObject::Desc &src, RenderObject::Desc::UpdateFlag updateFlags) {
    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Flags)) {
        desc.flags = src.flags;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Layer)) {
        desc.layer = src.layer;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Transform)) {
        desc.worldMatrix = src.worldMatrix;
        nonUniformScale = HasNonUniformScale(src.worldMatrix);
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Bounds)) {
        desc.aabb = src.aabb;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Mesh)) {
        desc.meshInstance = src.meshInstance;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::MeshLOD)) {
        UpdateMeshLOD(src.meshLOD);
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::FadeOpacity)) {
        UpdateFadeOpacity(src.fadeOpacity);
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Skeleton)) {
        desc.skeleton = src.skeleton;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Materials)) {
        UpdateMaterials(src.materials);
    }

    if (src.text) {
        if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Font)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.font = src.text->font;
        }

        if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Text)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.text = src.text->text;
        }

        if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::TextStyle)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.textRect = src.text->textRect;
            textDesc.textShadowOffset = src.text->textShadowOffset;
            textDesc.textShadowColor = src.text->textShadowColor;
            textDesc.textParams = src.text->textParams;
            textDesc.textScale = src.text->textScale;
            textDesc.textLineSpacing = src.text->textLineSpacing;
        }
    }

    if (BE1::HasAnyFlag(updateFlags, Desc::UpdateFlag::Transform | Desc::UpdateFlag::Bounds | Desc::UpdateFlag::Mesh)) {
        UpdateWorldAABB();
    }
}
