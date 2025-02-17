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
#include "Texture.h"

TextureManager      textureManager;

void TextureManager::Init() {
    textureHashMap.Init(1024, 1024, 1024);

    CreateEngineTextures();
}

void TextureManager::Shutdown() {
    textureHashMap.DeleteContents(true);
}

Texture *TextureManager::AllocTexture(const char *hashName) {
    if (textureHashMap.Get(hashName)) {
        BE_FATALERROR("%s texture already allocated", hashName);
    }

    Texture *texture = new Texture;
    texture->hashName = hashName;
    texture->name = hashName;
    texture->name.StripPath();
    texture->name.StripFileExtension();
    texture->index = textureHashMap.Count();
    texture->refCount = 1;

    textureHashMap.Set(texture->hashName, texture);

    return texture;
}

Texture *TextureManager::FindTexture(const char *hashName) const {
    const auto *entry = textureHashMap.Get(BE1::Str(hashName));
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

Texture *TextureManager::GetTexture(const char *hashName, Texture::Flag flags) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    Texture *texture = FindTexture(hashName);
    if (texture) {
        texture->refCount++;
        return texture;
    }

    texture = AllocTexture(hashName);
    if (!texture->Load(hashName, flags)) {
        DestroyTexture(texture);
        return defaultTexture;
    }

    return texture;
}

void TextureManager::ReleaseTexture(Texture *texture) {
    if (texture->refCount > 0) {
        if (--texture->refCount > 0) {
            return;
        }
    }

    if (BE1::HasFlag(texture->flags, Texture::Flag::Permanent)) {
        return;
    }

    DestroyTexture(texture);
}

void TextureManager::DestroyTexture(Texture *texture) {
    if (texture->refCount > 1) {
        BE_WARNLOG("TextureManager::DestroyTexture: texture '%s' has %i reference count\n", texture->hashName.c_str(), texture->refCount);
    }

    textureHashMap.Remove(texture->hashName);

    delete texture;
}

void TextureManager::DestroyUnusedTextures() {
    BE1::Array<Texture *> removeArray;

    for (const auto &entry : textureHashMap) {
        Texture *texture = entry.second;
        if (!texture) {
            continue;
        }

        if (!BE1::HasFlag(texture->flags, Texture::Flag::Permanent) && texture->refCount == 0) {
            removeArray.Append(texture);
        }
    }

    for (Texture *texture : removeArray) {
        DestroyTexture(texture);
    }
}

void TextureManager::CreateEngineTextures() {
    // Create default texture.
    defaultTexture = AllocTexture("_defaultTexture");
    defaultTexture->CreateDefaultTexture(16, Texture::Flag::Permanent);

    // Create white texture.
    whiteTexture = AllocTexture("_whiteTexture");
    whiteTexture->CreateColorTexture(8, BE1::Color4::white, Texture::Flag::Permanent);

    // Create flatNormal texture.
    flatNormalTexture = AllocTexture("_flatNormalTexture");
    flatNormalTexture->CreateFlatNormalTexture(16, Texture::Flag::Permanent);
}
