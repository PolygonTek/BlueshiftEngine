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
#include "Render/Anim.h"
#include "Render/Mesh.h"
#include "Skeleton.h"
#include "LoadingSystem.h"

//#define CYCLIC_DELTA_MOVEMENT

void Anim::Purge() {
    internalData.Purge();
}

void Anim::Reset() {
    BumpLoadSerial(); // 진행 중인 async completion 무효화
    state = State::Empty;

    Purge();
}

size_t Anim::Allocated() const {
    size_t size = internalData.Allocated() + hashName.Allocated() + name.Allocated();
    return size;
}

Anim &Anim::Copy(const Anim &other) {
    internalData.CopyFrom(other.internalData);
    internalData.isDefaultAnim = false;
    internalData.isAdditiveAnim = false;
    return *this;
}

void Anim::CreateDefaultAnim(const Skeleton *skeleton) {
    Purge();

    internalData.isDefaultAnim = true;

    internalData.numJoints = skeleton->NumJoints();
    internalData.numFrames = 1;

    internalData.length = 1000;

    internalData.numComponentsPerFrame = 0;

    internalData.joints.SetGranularity(1);
    internalData.joints.SetCount(internalData.numJoints);

    const Joint *skeletonJoints = skeleton->GetJoints();

    for (int jointIndex = 0; jointIndex < internalData.numJoints; jointIndex++) {
        JointInfo *ji = &internalData.joints[jointIndex];

        ji->nameIndex = animManager.JointIndexByName(skeleton->GetJointName(jointIndex));

        if (skeletonJoints[jointIndex].parent) {
            ji->parentIndex = (int32_t)(skeletonJoints[jointIndex].parent - skeletonJoints);
        } else {
            ji->parentIndex = -1;
        }
        ji->componentBits = 0;
        ji->componentOffset = 0;
    }
    
    internalData.baseFrame.SetGranularity(1);
    internalData.baseFrame.SetCount(internalData.numJoints);

    for (int i = 0; i < internalData.numJoints; i++) {
        internalData.baseFrame[i] = skeleton->GetBindPoses()[i];
    }

    internalData.frameTimes.SetGranularity(1);
    internalData.frameTimes.SetCount(1);
    internalData.frameTimes[0] = 0;

    ComputeTotalDelta();

    state = State::Ready;
}

std::shared_ptr<Anim> Anim::CreateAdditiveAnim(const char *hashName, const BE1::JointPose *firstFrame, int numJointIndexes, const int *jointIndexes) {
    assert(IsReady());

    std::shared_ptr<Anim> additiveAnim = animManager.AllocAnim(hashName);
    additiveAnim->Copy(*this);
    additiveAnim->internalData.isAdditiveAnim = true;

    BE1::JointPose *jointFrame = (BE1::JointPose *)_alloca16(internalData.numJoints * sizeof(jointFrame[0]));
    
    for (int i = 0; i < internalData.numFrames; i++) {
        GetSingleFrame(i, numJointIndexes, jointIndexes, jointFrame);

        for (int j = 0; j < numJointIndexes; j++) {
            int jointIndex = jointIndexes[j];
            const JointInfo *jointInfoPtr = &internalData.joints[jointIndex];

            if (jointInfoPtr->componentBits == 0) {
                continue;
            }

            jointFrame[jointIndex] -= firstFrame[jointIndex];

            float *componentPtr = &additiveAnim->internalData.components[jointInfoPtr->componentOffset + i * internalData.numComponentsPerFrame];

            if (jointInfoPtr->componentBits & (ComponentBit::Tx | ComponentBit::Ty | ComponentBit::Tz)) {
                if (jointInfoPtr->componentBits & ComponentBit::Tx) {
                    *componentPtr++ = jointFrame[jointIndex].t[0];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Ty) {
                    *componentPtr++ = jointFrame[jointIndex].t[1];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Tz) {
                    *componentPtr++ = jointFrame[jointIndex].t[2];
                }
            }

            if (jointInfoPtr->componentBits & (ComponentBit::Qx | ComponentBit::Qy | ComponentBit::Qz)) {
                if (jointInfoPtr->componentBits & ComponentBit::Qx) {
                    *componentPtr++ = jointFrame[jointIndex].q[0];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Qy) {
                    *componentPtr++ = jointFrame[jointIndex].q[1];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Qz) {
                    *componentPtr++ = jointFrame[jointIndex].q[2];
                }
            }

            if (jointInfoPtr->componentBits & (ComponentBit::Sx | ComponentBit::Sy | ComponentBit::Sz)) {
                if (jointInfoPtr->componentBits & ComponentBit::Sx) {
                    *componentPtr++ = jointFrame[jointIndex].s[0];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Sy) {
                    *componentPtr++ = jointFrame[jointIndex].s[1];
                }

                if (jointInfoPtr->componentBits & ComponentBit::Sz) {
                    *componentPtr++ = jointFrame[jointIndex].s[2];
                }
            }
        }
    }

    for (int j = 0; j < numJointIndexes; j++) {
        int jointIndex = jointIndexes[j];
        additiveAnim->internalData.baseFrame[jointIndex] -= firstFrame[jointIndex];
    }

    additiveAnim->state = State::Ready;
    return additiveAnim;
}

std::shared_ptr<Anim> Anim::CreateAdditiveAnim(const Skeleton *skeleton, int numJointIndexes, const int *jointIndexes) {
    assert(IsReady());
    assert(skeleton && skeleton->IsReady());

    if (NumJoints() != skeleton->NumJoints()) {
        BE_WARNLOG("different joint count for creating additive anim\n");
        return nullptr;
    }

    BE1::Str newName = hashName;
    newName += "-";
    newName += skeleton->GetHashName();

    const BE1::JointPose *firstFrame = skeleton->GetBindPoses();

    return CreateAdditiveAnim(newName.c_str(), firstFrame, numJointIndexes, jointIndexes);
}

std::shared_ptr<Anim> Anim::CreateAdditiveAnim(const Anim *refAnim, int numJointIndexes, const int *jointIndexes) {
    assert(IsReady());
    assert(refAnim && refAnim->IsReady());

    if (NumJoints() != refAnim->NumJoints()) {
        BE_WARNLOG("different joint count for creating additive anim\n");
        return nullptr;
    }

    BE1::Str newName = hashName;
    newName += "-";
    newName += refAnim->hashName;

    BE1::JointPose *firstFrame = (BE1::JointPose *)_alloca16(internalData.numJoints * sizeof(firstFrame[0]));
    refAnim->GetSingleFrame(0, numJointIndexes, jointIndexes, firstFrame);

    return CreateAdditiveAnim(newName.c_str(), firstFrame, numJointIndexes, jointIndexes);
}

bool Anim::Prepare(const char *filename, InternalData &outInternalData) {
    outInternalData.Purge();

    if (!outInternalData.LoadBinaryAnim(filename)) {
        return false;
    }

    outInternalData.isDefaultAnim = false;
    outInternalData.isAdditiveAnim = false;
    return true;
}

void Anim::ApplyPrepared(InternalData &newInternalData) {
    assert(state == State::Empty || state == State::Loading || state == State::Ready || state == State::Reloading);

    newInternalData.ResolveJointNames();

    Purge();

    internalData.Swap(newInternalData);
}

bool Anim::Load(const char *filename) {
    InternalData newInternalData;
    if (!Anim::Prepare(filename, newInternalData)) {
        return false;
    }

    BumpLoadSerial(); // 진행 중인 async completion 무효화
    state = State::Empty;

    ApplyPrepared(newInternalData);

    state = State::Ready;
    return true;
}

bool Anim::LoadAsync(const char *filename) {
    state = State::Loading;
    uint32_t newSerial = BumpLoadSerial();

    if (!loadingSystem.EnqueueAnimLoad(shared_from_this(), filename, false, newSerial)) {
        state = State::Failed;
        return false;
    }
    return true;
}

bool Anim::Reload() {
    if (internalData.isDefaultAnim || internalData.isAdditiveAnim) {
        return false;
    }
    if (state != State::Ready) {
        BE_WARNLOG("Anim::Reload: anim '%s' is not ready\n", GetHashName());
        return false;
    }

    return Load(hashName);
}

bool Anim::ReloadAsync() {
    if (internalData.isDefaultAnim || internalData.isAdditiveAnim) {
        return false;
    }

    if (!IsReady()) {
        BE_WARNLOG("Anim::ReloadAsync: anim '%s' is not ready\n", GetHashName());
        return false;
    }

    uint32_t newSerial = BumpLoadSerial();
    state = State::Reloading;

    if (!loadingSystem.EnqueueAnimLoad(shared_from_this(), hashName, true, newSerial)) {
        state = State::Ready;
        return false;
    }
    return true;
}

bool Anim::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    if (GetState() == State::Loading) {
        loadingSystem.BoostAnimLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (GetState() == State::Loading) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return IsReady();
}

bool Anim::WaitLoadComplete() {
    assert(BE1::Engine::IsInMainThread());

    if (IsLoading()) {
        loadingSystem.BoostAnimLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (IsLoading()) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return GetState() == State::Ready;
}

void Anim::Write(const char *filename) {
    assert(IsReady());

    internalData.WriteBinaryAnim(filename);
}

bool Anim::CheckHierarchy(const Skeleton *skeleton) const {
    assert(IsReady());
    assert(skeleton && skeleton->IsReady());

    // Checks if both have the same number of joints.
    if (internalData.joints.Count() != skeleton->NumJoints()) {
        BE_ERRLOG("Mesh '%s' has different number of joints than anim '%s'", skeleton->GetHashName(), hashName.c_str());
        return false;
    }

    const Joint *skeletonJoints = skeleton->GetJoints();

    const int numJoints = internalData.joints.Count();
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const JointInfo &joint = internalData.joints[jointIndex];
        const Joint &skeletonJoint = skeletonJoints[jointIndex];

        // Checks if both have the same joint names.
        if (skeletonJoint.name != animManager.JointNameByIndex(joint.nameIndex)) {
            BE_ERRLOG("Skeleton '%s''s joint names don't match anim '%s''s", skeleton->GetHashName(), hashName.c_str());
            return false;
        }

        int parentIndex;
        if (skeletonJoint.parent) {
            parentIndex = (int)(skeletonJoint.parent - skeletonJoints);
        } else {
            parentIndex = -1;
        }

        // Checks if both have the same parent-child relationships.
        if (parentIndex != joint.parentIndex) {
            BE_ERRLOG("Skeleton '%s' has different joint hierarchy than anim '%s'", skeleton->GetHashName(), hashName.c_str());
            return false;
        }
    }
    
    return true;
}

void Anim::ComputeTotalDelta() {
    if (!internalData.numComponentsPerFrame) {
        internalData.totalDelta.SetFromScalar(0);
    } else {
        const JointInfo &rootJoint = internalData.joints[0];

        const float *componentPtr = &internalData.components[rootJoint.componentOffset];

        if (rootJoint.componentBits & ComponentBit::Tx) {
            internalData.totalDelta.x = componentPtr[internalData.numComponentsPerFrame * (internalData.numFrames - 1)] - internalData.baseFrame[0].t[0];
            componentPtr++;
        } else {
            internalData.totalDelta.x = 0.0f;
        }

        if (rootJoint.componentBits & ComponentBit::Ty) {
            internalData.totalDelta.y = componentPtr[internalData.numComponentsPerFrame * (internalData.numFrames - 1)] - internalData.baseFrame[0].t[1];
            componentPtr++;
        } else {
            internalData.totalDelta.y = 0.0f;
        }

        if (rootJoint.componentBits & ComponentBit::Tz) {
            internalData.totalDelta.z = componentPtr[internalData.numComponentsPerFrame * (internalData.numFrames - 1)] - internalData.baseFrame[0].t[2];
        } else {
            internalData.totalDelta.z = 0.0f;
        }
    }

    BE_DLOG("animation '%s' total delta (%s)\n", name.c_str(), internalData.totalDelta.ToString(4));
}

void Anim::ComputeFrameAABBs(const Skeleton *skeleton, const Mesh *mesh, BE1::Array<BE1::AABB> &frameAABBs) const {
    assert(IsReady());
    assert(skeleton && skeleton->IsReady());
    assert(mesh && mesh->IsReady());

    if (skeleton->NumJoints() == 0) {
        return;
    }

    int *jointIndexes = (int *)_alloca16(internalData.numJoints * sizeof(int));
    int *jointParents = (int *)_alloca16(internalData.numJoints * sizeof(jointParents[0]));

    for (int jointIndex = 0; jointIndex < internalData.numJoints; jointIndex++) {
        jointIndexes[jointIndex] = jointIndex;
        jointParents[jointIndex] = internalData.joints[jointIndex].parentIndex;
    }
    
    BE1::JointPose *jointFrame = (BE1::JointPose *)_alloca16(internalData.numJoints * sizeof(jointFrame[0]));
    BE1::Mat3x4 *jointMats = (BE1::Mat3x4 *)_alloca16(internalData.numJoints * sizeof(jointMats[0]));
    BE1::Mat3x4 *jointMats2 = (BE1::Mat3x4 *)_alloca16(internalData.numJoints * sizeof(jointMats2[0]));
    
    frameAABBs.SetGranularity(1);
    frameAABBs.SetCount(internalData.numFrames);

    const int numSurfaces = mesh->NumSurfaces();

    for (int frameIndex = 0; frameIndex < internalData.numFrames; frameIndex++) {
        BE1::AABB &frameAABB = frameAABBs[frameIndex];

        GetSingleFrame(frameIndex, internalData.numJoints, jointIndexes, jointFrame);

        BE1::simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame, internalData.numJoints);

        BE1::simdProcessor->TransformJoints(jointMats, jointParents, 0, internalData.numJoints - 1);

        BE1::simdProcessor->MultiplyJoints(jointMats2, jointMats, skeleton->GetInvBindPoseMatrices(), internalData.numJoints);

        frameAABB.Clear();

        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const SubMesh *subMesh = mesh->GetSurface(surfaceIndex)->subMesh;
            const int numVerts = subMesh->NumVerts();

            switch (subMesh->MaxVertexWeights()) {
            case 8: {
                const BE1::VertexWeight8 *vertWeights = (const BE1::VertexWeight8 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                    const BE1::VertexWeight8 *vw = &vertWeights[vertexIndex];
                    const BE1::Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    BE1::Vec3 resultPos = BE1::Vec3::zero;

                    for (int weightIndex = 0; weightIndex < 8; weightIndex++) {
                        float w = vw->jointWeights[weightIndex];
                        if (w != 0.0f) {
                            if (sizeof(BE1::JointWeightType) == sizeof(byte)) {
                                w *= (1.0f / 255.0f);
                            }

                            resultPos += w * jointMats2[vw->jointIndexes[weightIndex]].TransformPos(pos);
                        }
                    }

                    frameAABB.AddPoint(resultPos);
                }
                break; }
            case 4: {
                const BE1::VertexWeight4 *vertWeights = (const BE1::VertexWeight4 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                    const BE1::VertexWeight4 *vw = &vertWeights[vertexIndex];
                    const BE1::Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    BE1::Vec3 resultPos = BE1::Vec3::zero;

                    for (int weightIndex = 0; weightIndex < 4; weightIndex++) {
                        float w = vw->jointWeights[weightIndex];
                        if (w != 0.0f) {
                            if (sizeof(BE1::JointWeightType) == sizeof(byte)) {
                                w *= (1.0f / 255.0f);
                            }
                            resultPos += w * jointMats2[vw->jointIndexes[weightIndex]].TransformPos(pos);
                        }
                    }

                    frameAABB.AddPoint(resultPos);
                }
                break; }
            case 1: {
                const BE1::VertexWeight1 *vertWeights = (const BE1::VertexWeight1 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                    const BE1::VertexWeight1 *vw = &vertWeights[vertexIndex];
                    const BE1::Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    BE1::Vec3 resultPos = jointMats2[vw->jointIndex].TransformPos(pos);

                    frameAABB.AddPoint(resultPos);
                }
                break; }
            }
        }
    }
}

void Anim::TimeToFrameInterpolation(int time, FrameInterpolation &frameInterpolation) const {
    assert(IsReady());

    if (internalData.numFrames <= 1) {
        // only one frame exists
        frameInterpolation.frame1 = 0;
        frameInterpolation.frame2 = 0;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.frontlerp = 1.0f;
        frameInterpolation.cycleCount = 0;
        return;
    }

    if (time <= 0) {
        // time is less than or equal to zero
        frameInterpolation.frame1 = 0;
        frameInterpolation.frame2 = 1;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.frontlerp = 1.0f;
        frameInterpolation.cycleCount = 0;
        return;
    }

    int lastFrameTime = internalData.frameTimes.Last();

    frameInterpolation.cycleCount = time / lastFrameTime;

    if (internalData.maxCycleCount > 0 && frameInterpolation.cycleCount >= internalData.maxCycleCount) {
        frameInterpolation.cycleCount = internalData.maxCycleCount - 1;
        frameInterpolation.frame1 = internalData.numFrames - 1;
        frameInterpolation.frame2 = frameInterpolation.frame1;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.frontlerp = 1.0f;
        return;
    }

    int t = time % lastFrameTime;

    int frameNum = BE1::BinSearch_LessEqual<int>(internalData.frameTimes.Ptr(), internalData.frameTimes.Count(), t);

    frameInterpolation.frame1 = frameNum;
    frameInterpolation.frame2 = frameInterpolation.frame1 + 1;

    int t1 = internalData.frameTimes[frameNum];
    int t2 = internalData.frameTimes[frameNum + 1];

    frameInterpolation.backlerp = (float)(t - t1) / (float)(t2 - t1);
    frameInterpolation.frontlerp = 1.0f - frameInterpolation.backlerp;
}

void Anim::GetTranslation(BE1::Vec3 &outTranslation, int time, bool isCyclicTranslation) const {
    assert(IsReady());

    //if (rootTranslationXY && rootTranslationZ) {
    //    outTranslation = baseFrame[0].t;
    //    return;
    //}

    FrameInterpolation frame;

    outTranslation[0] = internalData.baseFrame[0].t[0];
    outTranslation[1] = internalData.baseFrame[0].t[1];
    outTranslation[2] = internalData.baseFrame[0].t[2];

    const JointInfo &rootJoint = internalData.joints[0];

    if (!(rootJoint.componentBits & ComponentBit::Txyz)) {
        // just use the baseframe
        return;
    }

    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame2];

    if (rootJoint.componentBits & ComponentBit::Tx) {
        outTranslation.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (rootJoint.componentBits & ComponentBit::Ty) {
        outTranslation.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (rootJoint.componentBits & ComponentBit::Tz) {
        outTranslation.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
    }

    if (frame.cycleCount && isCyclicTranslation) {
        outTranslation += internalData.totalDelta * (float)frame.cycleCount;
    }

    /*if (rootTranslationXY) {
        outTranslation.x = baseFrame[0].t.x;
        outTranslation.y = baseFrame[0].t.y;
    }

    if (rootTranslationZ) {
        outTranslation.z = baseFrame[0].t.z;
    }*/
}

void Anim::GetRotation(BE1::Quat &outRotation, int time) const {
    assert(IsReady());

    if (internalData.hasRootRotation) {
        outRotation = internalData.baseFrame[0].q;
        return;
    }

    const JointInfo &rootJoint = internalData.joints[0];

    int componentBits = rootJoint.componentBits;
    if (!(componentBits & ComponentBit::Qxyz)) {
        // just use the baseframe
        outRotation[0] = internalData.baseFrame[0].q[0];
        outRotation[1] = internalData.baseFrame[0].q[1];
        outRotation[2] = internalData.baseFrame[0].q[2];
        outRotation[3] = internalData.baseFrame[0].q[3];
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame2];

    if (componentBits & ComponentBit::Tx) {
        componentPtr1++;
        componentPtr2++;
    }

    if (componentBits & ComponentBit::Ty) {
        componentPtr1++;
        componentPtr2++;
    }

    if (componentBits & ComponentBit::Tz) {
        componentPtr1++;
        componentPtr2++;
    }

    BE1::Quat q1;
    BE1::Quat q2;

    switch (componentBits & ComponentBit::Qxyz) {
    case ComponentBit::Qx:
        q1.x = componentPtr1[0];
        q2.x = componentPtr2[0];
        q1.y = internalData.baseFrame[0].q[1];
        q2.y = q1.y;
        q1.z = internalData.baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qy:
        q1.y = componentPtr1[0];
        q2.y = componentPtr2[0];
        q1.x = internalData.baseFrame[0].q[0];
        q2.x = q1.x;
        q1.z = internalData.baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qz:
        q1.z = componentPtr1[0];
        q2.z = componentPtr2[0];
        q1.x = internalData.baseFrame[0].q[0];
        q2.x = q1.x;
        q1.y = internalData.baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qx | ComponentBit::Qy:
        q1.x = componentPtr1[0];
        q1.y = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.y = componentPtr2[1];
        q1.z = internalData.baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qx | ComponentBit::Qz:
        q1.x = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.y = internalData.baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qy | ComponentBit::Qz:
        q1.y = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.y = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.x = internalData.baseFrame[0].q[0];
        q2.x = q1.x;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qx | ComponentBit::Qy | ComponentBit::Qz:
        q1.x = componentPtr1[0];
        q1.y = componentPtr1[1];
        q1.z = componentPtr1[2];
        q2.x = componentPtr2[0];
        q2.y = componentPtr2[1];
        q2.z = componentPtr2[2];
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    }

    outRotation.SetFromSlerp(q1, q2, frame.backlerp);
}

void Anim::GetScaling(BE1::Vec3 &outScaling, int time) const {
    assert(IsReady());

    const JointInfo &rootJoint = internalData.joints[0];

    outScaling = internalData.baseFrame[0].s;

    int componentBits = rootJoint.componentBits;
    if (!(componentBits & ComponentBit::Sxyz)) {
        // just use the baseframe
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &internalData.components[rootJoint.componentOffset + internalData.numComponentsPerFrame * frame.frame2];

    if (componentBits & ComponentBit::Txyz) {
        if (componentBits & ComponentBit::Tx) {
            componentPtr1++;
            componentPtr2++;
        }

        if (componentBits & ComponentBit::Ty) {
            componentPtr1++;
            componentPtr2++;
        }

        if (componentBits & ComponentBit::Tz) {
            componentPtr1++;
            componentPtr2++;
        }
    }

    if (componentBits & ComponentBit::Qxyz) {
        if (componentBits & ComponentBit::Qx) {
            componentPtr1++;
            componentPtr2++;
        }

        if (componentBits & ComponentBit::Qy) {
            componentPtr1++;
            componentPtr2++;
        }

        if (componentBits & ComponentBit::Qz) {
            componentPtr1++;
            componentPtr2++;
        }
    }

    if (rootJoint.componentBits & ComponentBit::Sx) {
        outScaling.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (rootJoint.componentBits & ComponentBit::Sy) {
        outScaling.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (rootJoint.componentBits & ComponentBit::Sz) {
        outScaling.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
    }
}

void Anim::GetAABB(BE1::AABB &outAabb, const BE1::Array<BE1::AABB> &frameAABBs, int time) const {
    assert(IsReady());

    if (frameAABBs.Count() == 0) {
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    if (frame.frame1 > frameAABBs.Count() - 1 || frame.frame2 > frameAABBs.Count() - 1) {
        BE_WARNLOG("Anim::GetAABB: AABB frame index out of range");
        return;
    }

    outAabb = frameAABBs[frame.frame1];
    outAabb.AddAABB(frameAABBs[frame.frame2]);

#if CYCLIC_DELTA_MOVEMENT
    if (frame.cycleCount) {
        BE1::Vec3 delta = internalData.totalDelta * (float)frame.cycleCount;
        outAabb[0] += delta;
        outAabb[1] += delta;
    }
    bool cyclicTranslation = true;
#else
    bool cyclicTranslation = false;
#endif
}

static void DecodeSingleFrame(const Anim::JointInfo *joints, int numJointIndexes, const int *jointIndexes, const float *frameComponents, BE1::JointPose *frame) {
    for (int i = 0; i < numJointIndexes; i++) {
        int jointIndex = jointIndexes[i];
        const Anim::JointInfo *jointInfoPtr = &joints[jointIndex];

        int componentBits = jointInfoPtr->componentBits;
        if (componentBits == 0) {
            continue;
        }

        BE1::JointPose *frameJointPtr = &frame[jointIndex];
        const float *jointComponentPtr = frameComponents + jointInfoPtr->componentOffset;

        if (componentBits & Anim::ComponentBit::Txyz) {
            if (componentBits & Anim::ComponentBit::Tx) {
                frameJointPtr->t.x = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Ty) {
                frameJointPtr->t.y = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Tz) {
                frameJointPtr->t.z = *jointComponentPtr++;
            }
        }

        if (componentBits & Anim::ComponentBit::Qxyz) {
            if (componentBits & Anim::ComponentBit::Qx) {
                frameJointPtr->q.x = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Qy) {
                frameJointPtr->q.y = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Qz) {
                frameJointPtr->q.z = *jointComponentPtr++;
            }

            frameJointPtr->q.w = frameJointPtr->q.CalcW();
        }

        if (componentBits & Anim::ComponentBit::Sxyz) {
            if (componentBits & Anim::ComponentBit::Sx) {
                frameJointPtr->s.x = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Sy) {
                frameJointPtr->s.y = *jointComponentPtr++;
            }

            if (componentBits & Anim::ComponentBit::Sz) {
                frameJointPtr->s.z = *jointComponentPtr++;
            }
        }
    }
}

void Anim::GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, BE1::JointPose *frame) const {
    assert(IsReady());

    // Copy the base frame
    BE1::simdProcessor->Memcpy(frame, internalData.baseFrame.Ptr(), internalData.baseFrame.Count() * sizeof(internalData.baseFrame[0]));

    if (frameNum == 0 || !internalData.numComponentsPerFrame) {
        // Just use the base frame
        return;
    }

    const float *frameComponents = &internalData.components[frameNum * internalData.numComponentsPerFrame];

    DecodeSingleFrame(internalData.joints.Ptr(), numJointIndexes, jointIndexes, frameComponents, frame);

    if (!internalData.hasRootTranslationXY) {
        frame[0].t.x = internalData.baseFrame[0].t.x;
        frame[0].t.y = internalData.baseFrame[0].t.y;
    }

    if (!internalData.hasRootTranslationZ) {
        frame[0].t.z = internalData.baseFrame[0].t.z;
    }

    if (!internalData.hasRootRotation) {
        frame[0].q = internalData.baseFrame[0].q;
    }
}

static int BE_FASTCALL DecodeInterpolatedFrame(const Anim::JointInfo *joints, int numJointIndexes, const int *jointIndexes, const float *frameComponents1, const float *frameComponents2, BE1::JointPose *frame, BE1::JointPose *blendFrame, int *lerpIndex) {
    int numLerpJoints = 0;

    for (int i = 0; i < numJointIndexes; i++) {
        int jointIndex = jointIndexes[i];
        const Anim::JointInfo *jointInfoPtr = &joints[jointIndex];

        int componentBits = jointInfoPtr->componentBits;
        if (componentBits == 0) {
            continue;
        }

        lerpIndex[numLerpJoints++] = jointIndex;

        BE1::JointPose *frameJointPtr = &frame[jointIndex];
        BE1::JointPose *blendJointPtr = &blendFrame[jointIndex];

        const float *jointComponentPtr1 = frameComponents1 + jointInfoPtr->componentOffset;
        const float *jointComponentPtr2 = frameComponents2 + jointInfoPtr->componentOffset;

        switch (componentBits & Anim::ComponentBit::Txyz) {
        case 0:
            blendJointPtr->t = frameJointPtr->t;
            break;
        case Anim::ComponentBit::Tx:
            frameJointPtr->t.x = jointComponentPtr1[0];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.y = frameJointPtr->t.y;
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Ty:
            frameJointPtr->t.y = jointComponentPtr1[0];
            blendJointPtr->t.y = jointComponentPtr2[0];
            blendJointPtr->t.x = frameJointPtr->t.x;
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Tz:
            frameJointPtr->t.z = jointComponentPtr1[0];
            blendJointPtr->t.z = jointComponentPtr2[0];
            blendJointPtr->t.x = frameJointPtr->t.x;
            blendJointPtr->t.y = frameJointPtr->t.y;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Tx | Anim::ComponentBit::Ty:
            frameJointPtr->t.x = jointComponentPtr1[0];
            frameJointPtr->t.y = jointComponentPtr1[1];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.y = jointComponentPtr2[1];
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Tx | Anim::ComponentBit::Tz:
            frameJointPtr->t.x = jointComponentPtr1[0];
            frameJointPtr->t.z = jointComponentPtr1[1];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.z = jointComponentPtr2[1];
            blendJointPtr->t.y = frameJointPtr->t.y;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Ty | Anim::ComponentBit::Tz:
            frameJointPtr->t.y = jointComponentPtr1[0];
            frameJointPtr->t.z = jointComponentPtr1[1];
            blendJointPtr->t.y = jointComponentPtr2[0];
            blendJointPtr->t.z = jointComponentPtr2[1];
            blendJointPtr->t.x = frameJointPtr->t.x;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Tx | Anim::ComponentBit::Ty | Anim::ComponentBit::Tz:
            frameJointPtr->t.x = jointComponentPtr1[0];
            frameJointPtr->t.y = jointComponentPtr1[1];
            frameJointPtr->t.z = jointComponentPtr1[2];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.y = jointComponentPtr2[1];
            blendJointPtr->t.z = jointComponentPtr2[2];
            jointComponentPtr1 += 3;
            jointComponentPtr2 += 3;
            break;
        }

        switch (componentBits & Anim::ComponentBit::Qxyz) {
        case 0:
            blendJointPtr->q = frameJointPtr->q;
            break;
        case Anim::ComponentBit::Qx:
            frameJointPtr->q.x = jointComponentPtr1[0];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.y = frameJointPtr->q.y;
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qy:
            frameJointPtr->q.y = jointComponentPtr1[0];
            blendJointPtr->q.y = jointComponentPtr2[0];
            blendJointPtr->q.x = frameJointPtr->q.x;
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qz:
            frameJointPtr->q.z = jointComponentPtr1[0];
            blendJointPtr->q.z = jointComponentPtr2[0];
            blendJointPtr->q.x = frameJointPtr->q.x;
            blendJointPtr->q.y = frameJointPtr->q.y;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qx | Anim::ComponentBit::Qy:
            frameJointPtr->q.x = jointComponentPtr1[0];
            frameJointPtr->q.y = jointComponentPtr1[1];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.y = jointComponentPtr2[1];
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qx | Anim::ComponentBit::Qz:
            frameJointPtr->q.x = jointComponentPtr1[0];
            frameJointPtr->q.z = jointComponentPtr1[1];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.z = jointComponentPtr2[1];
            blendJointPtr->q.y = frameJointPtr->q.y;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qy | Anim::ComponentBit::Qz:
            frameJointPtr->q.y = jointComponentPtr1[0];
            frameJointPtr->q.z = jointComponentPtr1[1];
            blendJointPtr->q.y = jointComponentPtr2[0];
            blendJointPtr->q.z = jointComponentPtr2[1];
            blendJointPtr->q.x = frameJointPtr->q.x;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Anim::ComponentBit::Qx | Anim::ComponentBit::Qy | Anim::ComponentBit::Qz:
            frameJointPtr->q.x = jointComponentPtr1[0];
            frameJointPtr->q.y = jointComponentPtr1[1];
            frameJointPtr->q.z = jointComponentPtr1[2];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.y = jointComponentPtr2[1];
            blendJointPtr->q.z = jointComponentPtr2[2];
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        }

        switch (componentBits & Anim::ComponentBit::Sxyz) {
        case 0:
            blendJointPtr->s = frameJointPtr->s;
            break;
        case Anim::ComponentBit::Sx:
            frameJointPtr->s.x = jointComponentPtr1[0];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.y = frameJointPtr->s.y;
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Sy:
            frameJointPtr->s.y = jointComponentPtr1[0];
            blendJointPtr->s.y = jointComponentPtr2[0];
            blendJointPtr->s.x = frameJointPtr->s.x;
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Sz:
            frameJointPtr->s.z = jointComponentPtr1[0];
            blendJointPtr->s.z = jointComponentPtr2[0];
            blendJointPtr->s.x = frameJointPtr->s.x;
            blendJointPtr->s.y = frameJointPtr->s.y;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Anim::ComponentBit::Sx | Anim::ComponentBit::Sy:
            frameJointPtr->s.x = jointComponentPtr1[0];
            frameJointPtr->s.y = jointComponentPtr1[1];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.y = jointComponentPtr2[1];
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Sx | Anim::ComponentBit::Sz:
            frameJointPtr->s.x = jointComponentPtr1[0];
            frameJointPtr->s.z = jointComponentPtr1[1];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.z = jointComponentPtr2[1];
            blendJointPtr->s.y = frameJointPtr->s.y;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Sy | Anim::ComponentBit::Sz:
            frameJointPtr->s.y = jointComponentPtr1[0];
            frameJointPtr->s.z = jointComponentPtr1[1];
            blendJointPtr->s.y = jointComponentPtr2[0];
            blendJointPtr->s.z = jointComponentPtr2[1];
            blendJointPtr->s.x = frameJointPtr->s.x;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Anim::ComponentBit::Sx | Anim::ComponentBit::Sy | Anim::ComponentBit::Sz:
            frameJointPtr->s.x = jointComponentPtr1[0];
            frameJointPtr->s.y = jointComponentPtr1[1];
            frameJointPtr->s.z = jointComponentPtr1[2];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.y = jointComponentPtr2[1];
            blendJointPtr->s.z = jointComponentPtr2[2];
            jointComponentPtr1 += 3;
            jointComponentPtr2 += 3;
            break;
        }
    }

    return numLerpJoints;
}

void Anim::GetInterpolatedFrame(FrameInterpolation &frameInterpolation, int numJointIndexes, const int *jointIndexes, BE1::JointPose *frame) const {
    assert(IsReady());

    // Copy the base frame
    BE1::simdProcessor->Memcpy(frame, internalData.baseFrame.Ptr(), internalData.baseFrame.Count() * sizeof(internalData.baseFrame[0]));

    if (!internalData.numComponentsPerFrame) {
        // Just use the base frame
        return;
    }

    BE1::JointPose *blendFrame = (BE1::JointPose *)_alloca16(internalData.baseFrame.Count() * sizeof(BE1::JointPose));
    int *lerpIndex = (int *)_alloca16(internalData.baseFrame.Count() * sizeof(lerpIndex[0]));

    const float *frameComponents1 = &internalData.components[frameInterpolation.frame1 * internalData.numComponentsPerFrame];
    const float *frameComponents2 = &internalData.components[frameInterpolation.frame2 * internalData.numComponentsPerFrame];

    int numLerpJoints = DecodeInterpolatedFrame(internalData.joints.Ptr(), numJointIndexes, jointIndexes, frameComponents1, frameComponents2, frame, blendFrame, lerpIndex);

    BE1::simdProcessor->BlendJoints(frame, blendFrame, frameInterpolation.backlerp, lerpIndex, numLerpJoints);

#if CYCLIC_DELTA_MOVEMENT
    if (frameInterpolation.cycleCount) {
        frame[0].t += internalData.totalDelta * (float)frameInterpolation.cycleCount;
    }
#endif

    if (!internalData.hasRootTranslationXY) {
        frame[0].t.x = internalData.baseFrame[0].t.x;
        frame[0].t.y = internalData.baseFrame[0].t.y;
    }

    if (!internalData.hasRootTranslationZ) {
        frame[0].t.z = internalData.baseFrame[0].t.z;
    }

    if (!internalData.hasRootRotation) {
        frame[0].q = internalData.baseFrame[0].q;
    }
}

static float CompareJointsT(const BE1::JointPose *joints1, const BE1::JointPose *joints2, int numJoints) {
    float maxDeltaT = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const BE1::JointPose *joint1Ptr = &joints1[i];
        const BE1::JointPose *joint2Ptr = &joints2[i];

        maxDeltaT = BE1::Max(maxDeltaT, joint1Ptr->t.DistanceSqr(joint2Ptr->t));
    }

    return maxDeltaT;
}

static float CompareJointsQ(const BE1::JointPose *joints1, const BE1::JointPose *joints2, int numJoints) {
    float maxDeltaQ = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const BE1::JointPose *joint1Ptr = &joints1[i];
        const BE1::JointPose *joint2Ptr = &joints2[i];

        // NOTE: w 값을 양수로 유지하기 위해 -1 을 곱한 xyz 를 고려해야 한다. 
        // 오차값이 실제보다 더 작게 나올 수도 있지만, 일단은 이렇게..
        maxDeltaQ = BE1::Max(maxDeltaQ, BE1::Math::Fabs(BE1::Math::Fabs(joint1Ptr->q.x) - BE1::Math::Fabs(joint2Ptr->q.x)));
        maxDeltaQ = BE1::Max(maxDeltaQ, BE1::Math::Fabs(BE1::Math::Fabs(joint1Ptr->q.y) - BE1::Math::Fabs(joint2Ptr->q.y)));
        maxDeltaQ = BE1::Max(maxDeltaQ, BE1::Math::Fabs(BE1::Math::Fabs(joint1Ptr->q.z) - BE1::Math::Fabs(joint2Ptr->q.z)));
    }

    return maxDeltaQ;
}

static float CompareJointsS(const BE1::JointPose *joints1, const BE1::JointPose *joints2, int numJoints) {
    float maxDeltaS = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const BE1::JointPose *joint1Ptr = &joints1[i];
        const BE1::JointPose *joint2Ptr = &joints2[i];

        // FIXME: compare method.
        maxDeltaS = BE1::Max(maxDeltaS, joint1Ptr->s.DistanceSqr(joint2Ptr->s));
    }

    return maxDeltaS;
}

void Anim::RemoveFrames(const BE1::Array<int> &removableFrameNums) {
    const int numRemovableFrameNums = removableFrameNums.Count();

    BE1::Array<float> newFrameComponents;
    newFrameComponents.SetGranularity(1);
    newFrameComponents.SetCount(internalData.components.Count() - numRemovableFrameNums * internalData.numComponentsPerFrame);

    BE1::Array<int> newFrameTimes;
    newFrameTimes.SetGranularity(1);
    newFrameTimes.SetCount(internalData.frameTimes.Count() - numRemovableFrameNums);

    int numNewFrames = 0;

    for (int frameNum = 0; frameNum < internalData.numFrames; frameNum++) {
        bool remove = false;
        for (int i = 0; i < numRemovableFrameNums; i++) {
            if (frameNum == removableFrameNums[i]) {
                remove = true;
                break;
            }
        }

        if (remove) {
            continue;
        }

        for (int i = 0; i < internalData.numComponentsPerFrame; i++) {
            newFrameComponents[numNewFrames * internalData.numComponentsPerFrame + i] = internalData.components[frameNum * internalData.numComponentsPerFrame + i];
        }
        newFrameTimes[numNewFrames] = internalData.frameTimes[frameNum];

        numNewFrames++;
    }

    internalData.components = newFrameComponents;
    internalData.frameTimes = newFrameTimes;

    internalData.numFrames = numNewFrames;
}

void Anim::ComputeRemovableFrames(const BE1::JointPose *frameJoints, const int *jointIndexes, BE1::JointPose *lerpedJoints, float epsilonT, float epsilonQ, float epsilonS, int frameNum1, int frameNum2, BE1::Array<int> &removableFrameNums) {
    if (frameNum1 + 1 == frameNum2) {
        return;
    }

    float blockTime = internalData.frameTimes[frameNum2] - internalData.frameTimes[frameNum1];

    float maxDt = 0.0f;
    float maxDq = 0.0f;
    float maxDs = 0.0f;

    int frameNumMaxDt = frameNum1;
    int frameNumMaxDq = frameNum1;
    int frameNumMaxDs = frameNum1;

    for (int frameNum = frameNum1 + 1; frameNum < frameNum2; frameNum++) {
        FrameInterpolation frameInterpolation;
        frameInterpolation.frame1 = frameNum1;
        frameInterpolation.frame2 = frameNum2;
        frameInterpolation.cycleCount = 0;
        frameInterpolation.backlerp = (internalData.frameTimes[frameNum] - internalData.frameTimes[frameNum1]) / blockTime;
        frameInterpolation.frontlerp = 1.0f - frameInterpolation.backlerp;
        GetInterpolatedFrame(frameInterpolation, internalData.numJoints, jointIndexes, lerpedJoints);

        float dt = CompareJointsT(&frameJoints[frameNum * internalData.numJoints], lerpedJoints, internalData.numJoints);
        if (dt > maxDt) {
            maxDt = dt;
            frameNumMaxDt = frameNum;
        }

        float dq = CompareJointsQ(&frameJoints[frameNum * internalData.numJoints], lerpedJoints, internalData.numJoints);
        if (dq > maxDq) {
            maxDq = dq;
            frameNumMaxDq = frameNum;
        }

        float ds = CompareJointsS(&frameJoints[frameNum * internalData.numJoints], lerpedJoints, internalData.numJoints);
        if (ds > maxDs) {
            maxDs = ds;
            frameNumMaxDs = frameNum;
        }
    }

    if (maxDt > epsilonT || maxDq > epsilonQ || maxDs > epsilonS) {
        int frameNum = BE1::Max3(frameNumMaxDt, frameNumMaxDq, frameNumMaxDs);

        ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum1, frameNum, removableFrameNums);
        ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum, frameNum2, removableFrameNums);
        return;
    }

    for (int i = frameNum1 + 1; i < frameNum2; i++) {
        removableFrameNums.Append(i);
    }
}

void Anim::OptimizeFrames(float epsilonT, float epsilonQ, float epsilonS) {
    if (internalData.numFrames <= 2 || !internalData.numComponentsPerFrame) {
        return;
    }

    // Set up whole joint indexes.
    int *jointIndexes = (int *)_alloca16(internalData.numJoints * sizeof(int));
    for (int i = 0; i < internalData.numJoints; i++) {
        jointIndexes[i] = i;
    }

    // Set up the reference joints of all frames.
    BE1::JointPose *frameJoints = (BE1::JointPose *)Mem_Alloc16(internalData.numFrames * internalData.numJoints * sizeof(BE1::JointPose));

    for (int frameNum = 0; frameNum < internalData.numFrames; frameNum++) {
        GetSingleFrame(frameNum, internalData.numJoints, jointIndexes, &frameJoints[frameNum * internalData.numJoints]);
    }

    BE1::JointPose *lerpedJoints = (BE1::JointPose *)_alloca16(internalData.numJoints * sizeof(BE1::JointPose));

    int frameNum1 = 0;
    int frameNum2 = internalData.numFrames - 1;

    BE1::Array<int> removableFrameNums;
    removableFrameNums.Reserve(internalData.numFrames);

    ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum1, frameNum2, removableFrameNums);

    if (removableFrameNums.Count() > 0) {
        BE_LOG("%.1f%% of frames removed\n", 100.0f * removableFrameNums.Count() / internalData.numFrames);

        RemoveFrames(removableFrameNums);
    }

    Mem_AlignedFree(frameJoints);
}
