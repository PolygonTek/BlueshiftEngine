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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Animation Controller", AnimControllerResource, Resource)
BEGIN_EVENTS(AnimControllerResource)
END_EVENTS

void AnimControllerResource::RegisterProperties() {
}

AnimControllerResource::AnimControllerResource() {
    animController = nullptr;
}

AnimControllerResource::~AnimControllerResource() {
    if (animController) {
        animControllerManager.ReleaseAnimController(animController, true);
    }
}

AnimController *AnimControllerResource::GetAnimController() {
    if (animController) {
        return animController;
    }
    const Str animControllerPath = resourceGuidMapper.Get(asset->GetGuid());
    animController = animControllerManager.GetAnimController(animControllerPath);
    return animController;
}

void AnimControllerResource::Rename(const Str &newName) {
    const Str animControllerPath = resourceGuidMapper.Get(asset->GetGuid());
    AnimController *existingAnimController = animControllerManager.FindAnimController(animControllerPath);
    if (existingAnimController) {
        animControllerManager.RenameAnimController(existingAnimController, newName);
    }
}

bool AnimControllerResource::Reload() {
    const Str animControllerPath = resourceGuidMapper.Get(asset->GetGuid());
    AnimController *existingAnimController = animControllerManager.FindAnimController(animControllerPath);
    if (existingAnimController) {
        existingAnimController->Reload();
        return true;
    }
    return false;
}

bool AnimControllerResource::Save() {
    const Str animControllerPath = resourceGuidMapper.Get(asset->GetGuid());
    AnimController *existingAnimController = animControllerManager.FindAnimController(animControllerPath);
    if (existingAnimController) {
        existingAnimController->Write(existingAnimController->GetHashName());
        return true;
    }
    return false;
}

BE_NAMESPACE_END
