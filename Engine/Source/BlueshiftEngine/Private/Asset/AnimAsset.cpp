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
#include "Render/Anim.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Anim", AnimAsset, Asset)
BEGIN_EVENTS(AnimAsset)
END_EVENTS

void AnimAsset::RegisterProperties() {
}

AnimAsset::AnimAsset() {
    anim = nullptr;
}

AnimAsset::~AnimAsset() {
    if (!anim) {
        const Str animPath = resourceGuidMapper.Get(GetGuid());
        anim = animManager.FindAnim(animPath);
    }

    if (anim) {
        animManager.ReleaseAnim(anim, true);
    }
}

Anim *AnimAsset::GetAnim() {
    if (anim) {
        return anim;
    }
    const Str animPath = resourceGuidMapper.Get(GetGuid());
    anim = animManager.GetAnim(animPath);
    return anim;
}

void AnimAsset::Rename(const Str &newName) {
    Anim *existingAnim = animManager.FindAnim(GetResourceFilename());
    if (existingAnim) {
        animManager.RenameAnim(existingAnim, newName);
    }

    Asset::Rename(newName);
}

void AnimAsset::Reload() {
    Anim *existingAnim = animManager.FindAnim(GetResourceFilename());
    if (existingAnim) {
        existingAnim->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
