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
BEGIN_PROPERTIES(TextureAsset)
END_PROPERTIES

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

void TextureAsset::Reload() {
    Texture *existingTexture = textureManager.FindTexture(GetResourceFilename());
    if (existingTexture) {
        existingTexture->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END