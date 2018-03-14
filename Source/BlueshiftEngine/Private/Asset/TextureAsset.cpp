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
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Texture", TextureAsset, Asset)
BEGIN_EVENTS(TextureAsset)
END_EVENTS

void TextureAsset::RegisterProperties() {
}

TextureAsset::TextureAsset() {
    texture = nullptr;
}

TextureAsset::~TextureAsset() {
    if (!texture) {
        const Str texturePath = resourceGuidMapper.Get(GetGuid());
        texture = textureManager.FindTexture(texturePath);
    }

    if (texture) {
        textureManager.ReleaseTexture(texture, true);
    }
}

Texture *TextureAsset::GetTexture() {
    if (texture) {
        return texture;
    }
    const Str texturePath = resourceGuidMapper.Get(GetGuid());
    texture = textureManager.GetTexture(texturePath);
    return texture;
}

void TextureAsset::Rename(const Str &newName) {
    Texture *existingTexture = textureManager.FindTexture(GetResourceFilename());
    if (existingTexture) {
        textureManager.RenameTexture(existingTexture, newName);
    }

    Asset::Rename(newName);
}

void TextureAsset::Reload() {
    Texture *existingTexture = textureManager.FindTexture(GetResourceFilename());
    if (existingTexture) {
        existingTexture->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
