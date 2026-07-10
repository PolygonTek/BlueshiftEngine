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

RenderObject::~RenderObject() {
}

void RenderObject::Update(const RenderObject::Desc &src, RenderObject::Desc::UpdateFlag flags) {
    if (BE1::HasFlag(flags, Desc::UpdateFlag::Transform)) {
        desc.worldMatrix = src.worldMatrix;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Bounds)) {
        desc.aabb = src.aabb;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Mesh)) {
        desc.meshInstance = src.meshInstance;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Materials)) {
        desc.materials = src.materials;
    }

    if (src.text) {
        if (BE1::HasFlag(flags, Desc::UpdateFlag::Font)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.font = src.text->font;
        }

        if (BE1::HasFlag(flags, Desc::UpdateFlag::Text)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.text = src.text->text;
        }

        if (BE1::HasFlag(flags, Desc::UpdateFlag::TextStyle)) {
            RenderObject::TextDesc &textDesc = desc.GetTextDesc();
            textDesc.textRect = src.text->textRect;
            textDesc.textShadowOffset = src.text->textShadowOffset;
            textDesc.textShadowColor = src.text->textShadowColor;
            textDesc.textParams = src.text->textParams;
            textDesc.textScale = src.text->textScale;
            textDesc.textLineSpacing = src.text->textLineSpacing;
        }
    }

    if (BE1::HasAnyFlag(flags, Desc::UpdateFlag::Transform | Desc::UpdateFlag::Bounds | Desc::UpdateFlag::Mesh)) {
        const BE1::AABB &localAABB = GetEffectiveLocalAABB(desc);
        if (localAABB.IsCleared()) {
            worldAABB = BE1::AABB::empty;
        } else {
            worldAABB.SetFromTransformedAABBFast(localAABB, desc.worldMatrix);
        }
    }
}
