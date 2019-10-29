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
#include "Components/ComImage.h"
#include "Render/Render.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Image", ComImage, Component)
BEGIN_EVENTS(ComImage)
END_EVENTS

void ComImage::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("sprite", "Sprite", Guid, GetSpriteGuid, SetSpriteGuid, GuidMapper::defaultTextureGuid,
        "", PropertyInfo::Flag::Editor).SetMetaObject(&TextureSpriteResource::metaObject);
}

ComImage::ComImage() {
    sprite = nullptr;
}

ComImage::~ComImage() {
    Purge(false);
}

void ComImage::Purge(bool chainPurge) {
    if (sprite) {
        textureManager.ReleaseTexture(sprite);
        sprite = nullptr;
    }
}

void ComImage::Init() {
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

Guid ComImage::GetSpriteGuid() const {
    if (sprite) {
        const Str spritePath = sprite->GetHashName();
        return resourceGuidMapper.Get(spritePath);
    }
    return Guid();
}

void ComImage::SetSpriteGuid(const Guid &guid) {
    if (sprite) {
        textureManager.ReleaseTexture(sprite);
    }

    const Str spritePath = resourceGuidMapper.Get(guid);
    sprite = textureManager.GetTexture(spritePath);
}

BE_NAMESPACE_END
