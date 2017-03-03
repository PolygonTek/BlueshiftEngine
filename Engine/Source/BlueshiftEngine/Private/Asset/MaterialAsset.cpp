#include "Precompiled.h"
#include "Render/Material.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Material", MaterialAsset, Asset)
BEGIN_EVENTS(MaterialAsset)
END_EVENTS
BEGIN_PROPERTIES(MaterialAsset)
END_PROPERTIES

MaterialAsset::MaterialAsset() {
    material = nullptr;
}

MaterialAsset::~MaterialAsset() {
    if (!material) {
        const Str materialPath = resourceGuidMapper.Get(GetGuid());
        material = materialManager.FindMaterial(materialPath);
    }

    if (material) {
        materialManager.ReleaseMaterial(material, true);
    }
}

Material *MaterialAsset::GetMaterial() {
    if (material) {
        return material;
    }
    const Str materialPath = resourceGuidMapper.Get(GetGuid());
    material = materialManager.GetMaterial(materialPath);
    return material;
}

void MaterialAsset::Reload() {
    Material *existingMaterial = materialManager.FindMaterial(GetResourceFilename());
    if (existingMaterial) {
        existingMaterial->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void MaterialAsset::Save() {
    Material *existingMaterial = materialManager.FindMaterial(GetAssetFilename());
    if (existingMaterial) {
        existingMaterial->Write(existingMaterial->GetHashName());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END