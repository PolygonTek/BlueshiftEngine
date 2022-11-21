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
#include "Core/Heap.h"
#include "Math/Math.h"
#include "Components/Transform/JointHierarchy.h"

BE_NAMESPACE_BEGIN

JointHierarchy::JointHierarchy(ComTransform *rootTransform, int numJoints) {
    this->numJoints = numJoints;
    this->localJointMatrices = (Mat3x4 *)Mem_Alloc16(numJoints * sizeof(Mat3x4));
    this->worldJointMatrices = (Mat3x4 *)Mem_Alloc16(numJoints * sizeof(Mat3x4));
    this->rootTransform = rootTransform;
}

JointHierarchy::~JointHierarchy() {
    if (localJointMatrices) {
        Mem_AlignedFree(localJointMatrices);
    }
    if (worldJointMatrices) {
        Mem_AlignedFree(worldJointMatrices);
    }
}

BE_NAMESPACE_END
