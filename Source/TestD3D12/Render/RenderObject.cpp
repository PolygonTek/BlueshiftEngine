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

RenderObject::~RenderObject() {
#ifdef USE_DBVT
    if (proxy) {
        Mem_Free(proxy);
    }
    if (meshSurfProxies) {
        Mem_Free(meshSurfProxies);
    }
#endif
}

void RenderObject::Update(const RenderObject::Desc &src, RenderObject::Desc::UpdateFlag flags) {
    if (BE1::HasFlag(flags, Desc::UpdateFlag::Transform)) {
        desc.worldMatrix = src.worldMatrix;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::AABB)) {
        desc.aabb = src.aabb;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Mesh)) {
        desc.mesh = src.mesh;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Textures)) {
        desc.textures = src.textures;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Font)) {
        desc.font = src.font;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::Text)) {
        desc.text = src.text;
    }

    if (BE1::HasFlag(flags, Desc::UpdateFlag::TextStyle)) {
        desc.textRect = src.textRect;
        desc.textShadowOffset = src.textShadowOffset;
        desc.textShadowColor = src.textShadowColor;
        desc.textParams = src.textParams;
        desc.textScale = src.textScale;
        desc.textLineSpacing = src.textLineSpacing;
    }

    if (BE1::HasAnyFlag(flags, Desc::UpdateFlag::Transform | Desc::UpdateFlag::AABB)) {
        worldAABB.SetFromTransformedAABBFast(desc.aabb, desc.worldMatrix);
    }
}
