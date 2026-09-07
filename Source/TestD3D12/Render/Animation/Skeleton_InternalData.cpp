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
#include "Skeleton.h"

Skeleton::InternalData &Skeleton::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void Skeleton::InternalData::Purge() {
    SAFE_DELETE_ARRAY(joints);
    numJoints = 0;

    if (jointParents) {
        Mem_AlignedFree(jointParents);
        jointParents = nullptr;
    }
    if (bindPoses) {
        Mem_AlignedFree(bindPoses);
        bindPoses = nullptr;
    }
    if (invBindPoseMats) {
        Mem_AlignedFree(invBindPoseMats);
        invBindPoseMats = nullptr;
    }
}

void Skeleton::InternalData::Swap(InternalData &rhs) {
    BE1::Swap(numJoints, rhs.numJoints);
    BE1::Swap(joints, rhs.joints);
    BE1::Swap(jointParents, rhs.jointParents);
    BE1::Swap(bindPoses, rhs.bindPoses);
    BE1::Swap(invBindPoseMats, rhs.invBindPoseMats);
}
