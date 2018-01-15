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
#include "AnimController/AnimController.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("AnimController", AnimControllerAsset, Asset)
BEGIN_EVENTS(AnimControllerAsset)
END_EVENTS

void AnimControllerAsset::RegisterProperties() {
}

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

void AnimControllerAsset::Rename(const Str &newName) {
    AnimController *existingAnimController = animControllerManager.FindAnimController(GetResourceFilename());
    if (existingAnimController) {
        animControllerManager.RenameAnimController(existingAnimController, newName);
    }

    Asset::Rename(newName);
}

void AnimControllerAsset::Reload() {
    AnimController *existingAnimController = animControllerManager.FindAnimController(GetResourceFilename());
    if (existingAnimController) {
        existingAnimController->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void AnimControllerAsset::Save() {
    AnimController *existingAnimController = animControllerManager.FindAnimController(GetResourceFilename());
    if (existingAnimController) {
        existingAnimController->Write(existingAnimController->GetHashName());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END
