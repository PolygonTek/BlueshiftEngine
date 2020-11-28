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
#include "Render/Texture.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Texture", TextureResource, Resource)
BEGIN_EVENTS(TextureResource)
END_EVENTS

void TextureResource::RegisterProperties() {
}

TextureResource::TextureResource() {
    texture = nullptr;
}

TextureResource::~TextureResource() {
    if (texture) {
        textureManager.ReleaseTexture(texture);
    }
}

Texture *TextureResource::GetTexture() {
    if (texture) {
        return texture;
    }
    const Str texturePath = resourceGuidMapper.Get(asset->GetGuid());
    texture = textureManager.GetTexture(texturePath);
    return texture;
}

void TextureResource::Rename(const Str &newName) {
    const Str texturePath = resourceGuidMapper.Get(asset->GetGuid());
    Texture *existingTexture = textureManager.FindTexture(texturePath);
    if (existingTexture) {
        textureManager.RenameTexture(existingTexture, newName);
    }
}

bool TextureResource::Reload() {
    const Str texturePath = resourceGuidMapper.Get(asset->GetGuid());
    Texture *existingTexture = textureManager.FindTexture(texturePath);
    if (existingTexture) {
        existingTexture->Reload();
        return true;
    }
    return false;
}

bool TextureResource::Save() {
    return false;
}

BE_NAMESPACE_END
