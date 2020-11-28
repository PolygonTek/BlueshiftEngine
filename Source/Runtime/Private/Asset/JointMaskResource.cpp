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
#include "Asset/Asset.h"
#include "Asset/Resource.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Joint Mask", JointMaskResource, Resource)
BEGIN_EVENTS(JointMaskResource)
END_EVENTS

void JointMaskResource::RegisterProperties() {
}

JointMaskResource::JointMaskResource() {}

JointMaskResource::~JointMaskResource() {}

void JointMaskResource::Rename(const Str &newName) {
}

bool JointMaskResource::Reload() {
    /*const Str jointMaskPath = resourceGuidMapper.Get(asset->GetGuid());
    JointMask *existingJointMask = jointMaskManager.FindJointMask(jointMaskPath);
    if (existingJointMask) {
        existingJointMask->Reload();
        return true;
    }*/
    return false;
}

bool JointMaskResource::Save() {
    return false;
}

BE_NAMESPACE_END
