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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Anim", AnimResource, Resource)
BEGIN_EVENTS(AnimResource)
END_EVENTS

void AnimResource::RegisterProperties() {
}

AnimResource::AnimResource() {
    anim = nullptr;
}

AnimResource::~AnimResource() {
    if (anim) {
        animManager.ReleaseAnim(anim);
    }
}

Anim *AnimResource::GetAnim() {
    if (anim) {
        return anim;
    }
    const Str animPath = resourceGuidMapper.Get(asset->GetGuid());
    anim = animManager.GetAnim(animPath);
    return anim;
}

void AnimResource::Rename(const Str &newName) {
    const Str animPath = resourceGuidMapper.Get(asset->GetGuid());
    Anim *existingAnim = animManager.FindAnim(animPath);
    if (existingAnim) {
        animManager.RenameAnim(existingAnim, newName);
    }
}

bool AnimResource::Reload() {
    const Str animPath = resourceGuidMapper.Get(asset->GetGuid());
    Anim *existingAnim = animManager.FindAnim(animPath);
    if (existingAnim) {
        existingAnim->Reload();
        return true;
    }
    return false;
}

bool AnimResource::Save() {
    Anim *existingAnim = animManager.FindAnim(asset->GetResourceFilename());
    if (existingAnim) {
        existingAnim->Write(existingAnim->GetHashName());
        return true;
    }
    return false;
}


BE_NAMESPACE_END
