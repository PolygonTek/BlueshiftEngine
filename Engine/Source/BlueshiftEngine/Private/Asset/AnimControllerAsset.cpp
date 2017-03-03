#include "Precompiled.h"
#include "AnimController/AnimController.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("AnimController", AnimControllerAsset, Asset)
BEGIN_EVENTS(AnimControllerAsset)
END_EVENTS
BEGIN_PROPERTIES(AnimControllerAsset)
END_PROPERTIES

AnimControllerAsset::AnimControllerAsset() {
    animController = nullptr;
}

AnimControllerAsset::~AnimControllerAsset() {
    if (!animController) {
        const Str animControllerPath = resourceGuidMapper.Get(GetGuid());
        animController = animControllerManager.FindAnimController(animControllerPath);
    }

    if (animController) {
        animControllerManager.ReleaseAnimController(animController, true);
    }
}

AnimController *AnimControllerAsset::GetAnimController() {
    if (animController) {
        return animController;
    }
    const Str animControllerPath = resourceGuidMapper.Get(GetGuid());
    animController = animControllerManager.GetAnimController(animControllerPath);
    return animController;
}

void AnimControllerAsset::Reload() {
    AnimController *existingAnimController = animControllerManager.FindAnimController(GetResourceFilename());
    if (existingAnimController) {
        existingAnimController->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void AnimControllerAsset::Save() {
    AnimController *existingAnimController = animControllerManager.FindAnimController(GetAssetFilename());
    if (existingAnimController) {
        existingAnimController->Write(existingAnimController->GetHashName());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END
