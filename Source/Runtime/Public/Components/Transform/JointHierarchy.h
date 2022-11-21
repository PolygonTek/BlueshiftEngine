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

#pragma once

BE_NAMESPACE_BEGIN

class Mat3x4;
class ComTransform;

class JointHierarchy {
    friend class ComTransform;

public:
    JointHierarchy(ComTransform *inRootTransform, int inNumJoints);
    JointHierarchy() = delete;
    ~JointHierarchy();

    JointHierarchy *        AddRefCount() { refCount++; return this; }
    int                     GetRefCount() const { return refCount; }

    static void             Release(JointHierarchy *jointHierarchy) { if (--jointHierarchy->refCount == 0) delete jointHierarchy; }

    const ComTransform *    GetRootTransform() const { return rootTransform; }

    int                     NumJoints() const { return numJoints; }

    Mat3x4 *                GetLocalJointMatrices() { return localJointMatrices; }
    Mat3x4 &                GetLocalJointMatrix(int jointIndex);

    Mat3x4 *                GetWorldJointMatrices() { return worldJointMatrices; }
    Mat3x4 &                GetWorldJointMatrix(int jointIndex);

private:
    const ComTransform *    rootTransform = nullptr;
    Mat3x4 *                localJointMatrices = nullptr;
    Mat3x4 *                worldJointMatrices = nullptr;
    int                     numJoints = 0;
    int                     refCount = 0;
};

BE_INLINE Mat3x4 &JointHierarchy::GetLocalJointMatrix(int jointIndex) {
    assert(jointIndex >= 0 && jointIndex < numJoints);
    return localJointMatrices[jointIndex];
}

BE_INLINE Mat3x4 &JointHierarchy::GetWorldJointMatrix(int jointIndex) {
    assert(jointIndex >= 0 && jointIndex < numJoints);
    return worldJointMatrices[jointIndex];
}

BE_NAMESPACE_END
