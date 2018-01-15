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
#include "Render/Render.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Simd/Simd.h"

//#define CYCLIC_DELTA_MOVEMENT

BE_NAMESPACE_BEGIN

size_t Anim::Allocated() const {
    size_t size = jointInfo.Allocated() + frameComponents.Allocated() + frameToTimeMap.Allocated() + timeToFrameMap.Allocated() + hashName.Allocated();
    return size;
}

void Anim::Purge() {
    numFrames = 0;
    numJoints = 0;
    animLength = 0;
    maxCycleCount = 1;

    rootRotation = false;
    rootTranslationXY = false;
    rootTranslationZ = false;

    totalDelta.SetFromScalar(0);
    
    jointInfo.Clear();
    frameComponents.Clear();
    frameToTimeMap.Clear();
    timeToFrameMap.Clear();
}

Anim &Anim::Copy(const Anim &other) {
    numJoints = other.numJoints;
    numFrames = other.numFrames;
    numAnimatedComponents = other.numAnimatedComponents;
    animLength = other.animLength;
    maxCycleCount = other.maxCycleCount;

    rootRotation = other.rootRotation;
    rootTranslationXY = other.rootTranslationXY;
    rootTranslationZ = other.rootTranslationZ;

    jointInfo = other.jointInfo;
    baseFrame = other.baseFrame;
    frameComponents = other.frameComponents;
    frameToTimeMap = other.frameToTimeMap;
    timeToFrameMap = other.timeToFrameMap;
    totalDelta = other.totalDelta;

    return *this;
}

void Anim::CreateDefaultAnim(const Skeleton *skeleton) {
    isDefaultAnim = true;

    numJoints = skeleton->NumJoints();
    numFrames = 1;

    animLength = 1000;

    numAnimatedComponents = 0;

    jointInfo.SetGranularity(1);
    jointInfo.SetCount(numJoints);

    const Joint *joints = skeleton->GetJoints();

    for (int i = 0; i < numJoints; i++) {
        JointInfo *jai = &jointInfo[i];
        jai->nameIndex = animManager.JointIndexByName(skeleton->GetJointName(i));
        if (joints[i].parent) {
            jai->parentNum = (int32_t)(joints[i].parent - joints);
        } else {
            jai->parentNum = -1;
        }
        jai->animBits = 0;
        jai->firstComponent = 0;
    }
    
    baseFrame.SetGranularity(1);
    baseFrame.SetCount(numJoints);

    for (int i = 0; i < numJoints; i++) {
        baseFrame[i] = skeleton->GetBindPoses()[i];
    }

    frameToTimeMap.SetGranularity(1);
    frameToTimeMap.SetCount(1);
    frameToTimeMap[0] = 0;

    ComputeTimeFrames();

    ComputeTotalDelta();
}

Anim *Anim::CreateAdditiveAnim(const char *hashName, const JointPose *firstFrame, int numJointIndexes, const int *jointIndexes) {
    Anim *additiveAnim = animManager.AllocAnim(hashName);
    additiveAnim->Copy(*this);

    isAdditiveAnim = true;

    JointPose *jointFrame = (JointPose *)_alloca16(numJoints * sizeof(jointFrame[0]));
    
    for (int i = 0; i < numFrames; i++) {
        GetSingleFrame(i, numJointIndexes, jointIndexes, jointFrame);

        for (int j = 0; j < numJointIndexes; j++) {
            int jointIndex = jointIndexes[j];
            const JointInfo *infoPtr = &jointInfo[jointIndex];

            if (infoPtr->animBits == 0) {
                continue;
            }

            jointFrame[jointIndex] -= firstFrame[jointIndex];

            float *componentPtr = &additiveAnim->frameComponents[i * numAnimatedComponents + infoPtr->firstComponent];

            if (infoPtr->animBits & (Tx | Ty | Tz)) {
                if (infoPtr->animBits & Tx) {
                    *componentPtr++ = jointFrame[jointIndex].t[0];
                }

                if (infoPtr->animBits & Ty) {
                    *componentPtr++ = jointFrame[jointIndex].t[1];
                }

                if (infoPtr->animBits & Tz) {
                    *componentPtr++ = jointFrame[jointIndex].t[2];
                }
            }

            if (infoPtr->animBits & (Qx | Qy | Qz)) {
                if (infoPtr->animBits & Qx) {
                    *componentPtr++ = jointFrame[jointIndex].q[0];
                }

                if (infoPtr->animBits & Qy) {
                    *componentPtr++ = jointFrame[jointIndex].q[1];
                }

                if (infoPtr->animBits & Qz) {
                    *componentPtr++ = jointFrame[jointIndex].q[2];
                }
            }

            if (infoPtr->animBits & (Sx | Sy | Sz)) {
                if (infoPtr->animBits & Sx) {
                    *componentPtr++ = jointFrame[jointIndex].s[0];
                }

                if (infoPtr->animBits & Sy) {
                    *componentPtr++ = jointFrame[jointIndex].s[1];
                }

                if (infoPtr->animBits & Sz) {
                    *componentPtr++ = jointFrame[jointIndex].s[2];
                }
            }
        }
    }

    for (int j = 0; j < numJointIndexes; j++) {
        int jointIndex = jointIndexes[j];
        additiveAnim->baseFrame[jointIndex] -= firstFrame[jointIndex];
    }

    return additiveAnim;
}

Anim *Anim::CreateAdditiveAnim(const Skeleton *skeleton, int numJointIndexes, const int *jointIndexes) {
    if (NumJoints() != skeleton->NumJoints()) {
        BE_WARNLOG(L"different joint count for creating additive anim\n");
        return nullptr;
    }

    Str newName = hashName;
    newName += "-";
    newName += skeleton->GetHashName();

    const Joint *joints = skeleton->GetJoints();
    const JointPose *firstFrame = skeleton->GetBindPoses();

    return CreateAdditiveAnim(newName.c_str(), firstFrame, numJointIndexes, jointIndexes);
}

Anim *Anim::CreateAdditiveAnim(const Anim *refAnim, int numJointIndexes, const int *jointIndexes) {
    if (NumJoints() != refAnim->NumJoints()) {
        BE_WARNLOG(L"different joint count for creating additive anim\n");
        return nullptr;
    }

    Str newName = hashName;
    newName += "-";
    newName += refAnim->hashName;

    JointPose *firstFrame = (JointPose *)_alloca16(numJoints * sizeof(firstFrame[0]));	
    refAnim->GetSingleFrame(0, numJointIndexes, jointIndexes, firstFrame);

    return CreateAdditiveAnim(newName.c_str(), firstFrame, numJointIndexes, jointIndexes);
}

bool Anim::Load(const char *filename) {
    Purge();

    Str bAnimFilename = filename;
    if (!Str::CheckExtension(filename, ".banim")) {
        bAnimFilename.SetFileExtension(".banim");
    }

    BE_LOG(L"Loading anim '%hs'...\n", bAnimFilename.c_str());

    bool ret = LoadBinaryAnim(filename);
    if (!ret) {
        return false;
    }

    isDefaultAnim = false;
    isAdditiveAnim = false;
    
    return ret;
}

bool Anim::Reload() {
    if (isDefaultAnim || isAdditiveAnim) {
        return false;
    }

    Str _hashName = hashName;
    return Load(_hashName);
}

void Anim::Write(const char *filename) {
    WriteBinaryAnim(filename);
}

bool Anim::CheckHierarchy(const Skeleton *skeleton) const {
    if (jointInfo.Count() != skeleton->NumJoints()) {
        BE_ERRLOG(L"Mesh '%hs' has different number of joints than anim '%hs'", skeleton->GetHashName(), hashName.c_str());
        return false;
    }

    const Joint *otherJoints = skeleton->GetJoints();
    for (int i = 0; i < jointInfo.Count(); i++) {
        int jointNum = jointInfo[i].nameIndex;
        if (otherJoints[i].name != animManager.JointNameByIndex(jointNum)) {
            BE_ERRLOG(L"Skeleton '%hs''s joint names don't match anim '%hs''s", skeleton->GetHashName(), hashName.c_str());
            return false;
        }

        int	parent;
        if (otherJoints[i].parent)  {
            parent = (int)(otherJoints[i].parent - otherJoints);
        } else {
            parent = -1;
        }

        if (parent != jointInfo[i].parentNum) {
            BE_ERRLOG(L"Skeleton '%hs' has different joint hierarchy than anim '%hs'", skeleton->GetHashName(), hashName.c_str());
            return false;
        }
    }
    
    return true;
}

void Anim::ComputeTotalDelta() {
    if (!numAnimatedComponents) {
        totalDelta.SetFromScalar(0);
    } else {
        float *componentPtr = &frameComponents[jointInfo[0].firstComponent];
        if (jointInfo[0].animBits & Tx) {
            totalDelta.x = componentPtr[numAnimatedComponents * (numFrames - 1)] - baseFrame[0].t[0];
            componentPtr++;
        } else {
            totalDelta.x = 0.0f;
        }

        if (jointInfo[0].animBits & Ty) {
            totalDelta.y = componentPtr[numAnimatedComponents * (numFrames - 1)] - baseFrame[0].t[1];
            componentPtr++;
        } else {
            totalDelta.y = 0.0f;
        }

        if (jointInfo[0].animBits & Tz) {
            totalDelta.z = componentPtr[numAnimatedComponents * (numFrames - 1)] - baseFrame[0].t[2];
        } else {
            totalDelta.z = 0.0f;
        }
    }

    BE_DLOG(L"animation '%hs' total delta (%.4f, %.4f, %.4f)\n", name.c_str(), totalDelta.x, totalDelta.y, totalDelta.z);
}

void Anim::ComputeTimeFrames() {
    timeToFrameMap.Clear();

    int lastFrameNum = numFrames - 1;
    int lastFrameTime = frameToTimeMap[lastFrameNum];
    timeToFrameMap.SetCount(lastFrameTime / 100 + 1);

    for (int t = 0; t < lastFrameTime; t += 100) {
        int i = lastFrameNum;
        for (; i >= 0; i--) {
            if (t >= frameToTimeMap[i]) {
                break;
            }
        }

        timeToFrameMap[t / 100] = i;
    }
}

void Anim::ComputeFrameAABBs(const Skeleton *skeleton, const Mesh *mesh, Array<AABB> &frameAABBs) const {
    if (skeleton->NumJoints() == 0) {
        return;
    }

    int *jointIndexes = (int *)_alloca16(numJoints * sizeof(int));
    int *jointParents = (int *)_alloca16(numJoints * sizeof(jointParents[0]));
    for (int i = 0; i < numJoints; i++) {
        jointIndexes[i] = i;
        jointParents[i] = jointInfo[i].parentNum;
    }
    
    JointPose *jointFrame = (JointPose *)_alloca16(numJoints * sizeof(jointFrame[0]));
    Mat3x4 *jointMats = (Mat3x4 *)_alloca16(numJoints * sizeof(jointMats[0]));
    Mat3x4 *jointMats2 = (Mat3x4 *)_alloca16(numJoints * sizeof(jointMats2[0]));
    
    frameAABBs.SetGranularity(1);
    frameAABBs.SetCount(numFrames);

    for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        AABB &frameAABB = frameAABBs[frameIndex];

        GetSingleFrame(frameIndex, numJoints, jointIndexes, jointFrame);

        simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame, numJoints);

        simdProcessor->TransformJoints(jointMats, jointParents, 1, numJoints - 1);

        simdProcessor->MultiplyJoints(jointMats2, jointMats, skeleton->GetInvBindPoseMats(), numJoints);

        frameAABB.Clear();

        for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
            const SubMesh *subMesh = mesh->GetSurface(surfaceIndex)->subMesh;

            switch (subMesh->MaxVertexWeights()) {
            case 8:
                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                    const VertexWeight8 *vertWeights = (const VertexWeight8 *)subMesh->VertWeights();
                    const VertexWeight8 *vw = &vertWeights[vertexIndex];
                    const Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    Vec3 resultPos = Vec3::zero;

                    for (int weightIndex = 0; weightIndex < 8; weightIndex++) {
                        float w = vw->jointWeights[weightIndex];
                        if (w != 0.0f) {
                            if (sizeof(JointWeightType) == sizeof(byte)) {
                                w *= 1.0f / 255.0f;
                            }

                            resultPos += w * jointMats2[vw->jointIndexes[weightIndex]].Transform(pos);
                        }
                    }

                    frameAABB.AddPoint(resultPos);
                }
                break;
            case 4:
                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                    const VertexWeight4 *vertWeights = (const VertexWeight4 *)subMesh->VertWeights();
                    const VertexWeight4 *vw = &vertWeights[vertexIndex];
                    const Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    Vec3 resultPos = Vec3::zero;

                    for (int weightIndex = 0; weightIndex < 4; weightIndex++) {
                        float w = vw->jointWeights[weightIndex];
                        if (w != 0.0f) {
                            if (sizeof(JointWeightType) == sizeof(byte)) {
                                w *= 1.0f / 255.0f;
                            }
                            resultPos += w * jointMats2[vw->jointIndexes[weightIndex]].Transform(pos);
                        }
                    }

                    frameAABB.AddPoint(resultPos);
                }
                break;
            case 1:
                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                    const VertexWeight1 *vertWeights = (const VertexWeight1 *)subMesh->VertWeights();
                    const VertexWeight1 *vw = &vertWeights[vertexIndex];
                    const Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    Vec3 resultPos = jointMats2[vw->jointIndex].Transform(pos);

                    frameAABB.AddPoint(resultPos);
                }
                break;
            }
        }
    }
}

void Anim::TimeToFrameInterpolation(int time, FrameInterpolation &frameInterpolation) const {
    // only one frame exists
    if (numFrames <= 1) {
        frameInterpolation.frame1 = 0;
        frameInterpolation.frame2 = 0;
        frameInterpolation.frontlerp = 1.0f;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.cycleCount = 0;
        return;
    }

    if (time <= 0) {
        frameInterpolation.frame1 = 0;
        frameInterpolation.frame2 = 1;
        frameInterpolation.frontlerp = 1.0f;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.cycleCount = 0;
        return;
    }

    int lastFrameTime = frameToTimeMap[frameToTimeMap.Count() - 1];

    frameInterpolation.cycleCount = time / lastFrameTime;

    // time 이 maxCycleCount 를 넘어갔다면
    if (maxCycleCount > 0 && frameInterpolation.cycleCount >= maxCycleCount) {
        frameInterpolation.cycleCount = maxCycleCount - 1;
        frameInterpolation.frame1 = numFrames - 1;
        frameInterpolation.frame2 = frameInterpolation.frame1;
        frameInterpolation.frontlerp = 1.0f;
        frameInterpolation.backlerp = 0.0f;
        return;
    }
    
    time = time % lastFrameTime;

    int frameNum = timeToFrameMap[time / 100];
    for (; frameNum < frameToTimeMap.Count() - 1; frameNum++) {
        if (time < frameToTimeMap[frameNum + 1]) {
            break;
        }
    }

    frameInterpolation.frame1 = frameNum;
    frameInterpolation.frame2 = frameInterpolation.frame1 + 1;

    int t1 = frameToTimeMap[frameNum];
    int t2 = frameToTimeMap[frameNum + 1];

    frameInterpolation.backlerp = (float)(time - t1) / (float)(t2 - t1);
    frameInterpolation.frontlerp = 1.0f - frameInterpolation.backlerp;
}

void Anim::GetTranslation(Vec3 &outTranslation, int time, bool cyclicTranslation) const {
    //if (rootTranslationXY && rootTranslationZ) {
    //    outTranslation = baseFrame[0].t;
    //    return;
    //}

    FrameInterpolation frame;

    outTranslation[0] = baseFrame[0].t[0];
    outTranslation[1] = baseFrame[0].t[1];
    outTranslation[2] = baseFrame[0].t[2];

    if (!(jointInfo[0].animBits & (Tx | Ty | Tz))) {
        // just use the baseframe
        return;
    }

    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &frameComponents[numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent];
    const float *componentPtr2 = &frameComponents[numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent];

    if (jointInfo[0].animBits & Tx) {
        outTranslation.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (jointInfo[0].animBits & Ty) {
        outTranslation.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (jointInfo[0].animBits & Tz) {
        outTranslation.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
    }

    if (frame.cycleCount && cyclicTranslation) {
        outTranslation += totalDelta * (float)frame.cycleCount;
    }

    /*if (rootTranslationXY) {
        outTranslation.x = baseFrame[0].t.x;
        outTranslation.y = baseFrame[0].t.y;
    }

    if (rootTranslationZ) {
        outTranslation.z = baseFrame[0].t.z;
    }*/
}

void Anim::GetRotation(Quat &outRotation, int time) const {
    if (rootRotation) {
        outRotation = baseFrame[0].q;
        return;
    }

    int animBits = jointInfo[0].animBits;
    if (!(animBits & (Qx | Qy | Qz))) {
        // just use the baseframe
        outRotation[0] = baseFrame[0].q[0];
        outRotation[1] = baseFrame[0].q[1];
        outRotation[2] = baseFrame[0].q[2];
        outRotation[3] = baseFrame[0].q[3];
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &frameComponents[numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent];
    const float *componentPtr2 = &frameComponents[numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent];

    if (animBits & Tx) {
        componentPtr1++;
        componentPtr2++;
    }

    if (animBits & Ty) {
        componentPtr1++;
        componentPtr2++;
    }

    if (animBits & Tz) {
        componentPtr1++;
        componentPtr2++;
    }

    Quat q1;
    Quat q2;

    switch (animBits & (Qx | Qy | Qz)) {
    case Qx:
        q1.x = componentPtr1[0];
        q2.x = componentPtr2[0];
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qy:
        q1.y = componentPtr1[0];
        q2.y = componentPtr2[0];
        q1.x = baseFrame[0].q[0];
        q2.x = q1.x;
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qz:
        q1.z = componentPtr1[0];
        q2.z = componentPtr2[0];
        q1.x = baseFrame[0].q[0];
        q2.x = q1.x;
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qx | Qy:
        q1.x = componentPtr1[0];
        q1.y = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.y = componentPtr2[1];
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qx | Qz:
        q1.x = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qy | Qz:
        q1.y = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.y = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.x = baseFrame[0].q[0];
        q2.x = q1.x;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case Qx | Qy | Qz:
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

void Anim::GetScaling(Vec3 &outScaling, int time) const {
    int animBits = jointInfo[0].animBits;
    if (!(animBits & (Sx | Sy | Sz))) {
        // just use the baseframe
        outScaling = baseFrame[0].s;
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &frameComponents[numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent];
    const float *componentPtr2 = &frameComponents[numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent];

    if (animBits & (Tx | Ty | Tz)) {
        if (animBits & Tx) {
            componentPtr1++;
            componentPtr2++;
        }

        if (animBits & Ty) {
            componentPtr1++;
            componentPtr2++;
        }

        if (animBits & Tz) {
            componentPtr1++;
            componentPtr2++;
        }
    }

    if (animBits & (Qx | Qy | Qz)) {
        if (animBits & Qx) {
            componentPtr1++;
            componentPtr2++;
        }

        if (animBits & Qy) {
            componentPtr1++;
            componentPtr2++;
        }

        if (animBits & Qz) {
            componentPtr1++;
            componentPtr2++;
        }
    }

    if (jointInfo[0].animBits & Sx) {
        outScaling.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (jointInfo[0].animBits & Sy) {
        outScaling.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
        componentPtr1++;
        componentPtr2++;
    }

    if (jointInfo[0].animBits & Sz) {
        outScaling.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
    }
}

void Anim::GetAABB(AABB &outAabb, const Array<AABB> &frameAABBs, int time) const {
    if (frameAABBs.Count() == 0) {
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    if (frame.frame1 > frameAABBs.Count() - 1 || frame.frame2 > frameAABBs.Count() - 1) {
        BE_WARNLOG(L"Anim::GetAABB: AABB frame index out of range");
        return;
    }

    outAabb = frameAABBs[frame.frame1];
    outAabb.AddAABB(frameAABBs[frame.frame2]);

#if CYCLIC_DELTA_MOVEMENT
    if (frame.cycleCount) {
        Vec3 delta = totalDelta * (float)frame.cycleCount;
        outAabb[0] += delta;
        outAabb[1] += delta;
    }
    bool cyclicTranslation = true;
#else
    bool cyclicTranslation = false;
#endif
}

void Anim::GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, JointPose *joints) const {
    // copy the baseframe
    simdProcessor->Memcpy(joints, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (frameNum == 0 || !numAnimatedComponents) {
        // just use the base frame
        return;
    }

    const float *frame = &frameComponents[frameNum * numAnimatedComponents];

    for (int i = 0; i < numJointIndexes; i++) {
        int j = jointIndexes[i];
        const JointInfo *infoPtr = &jointInfo[j];

        int animBits = infoPtr->animBits;
        if (animBits == 0) {
            continue;
        }

        JointPose *jointPtr = &joints[j];
        const float *jointframe = frame + infoPtr->firstComponent;

        if (animBits & (Tx | Ty | Tz)) {
            if (animBits & Tx) {
                jointPtr->t.x = *jointframe++;
            }

            if (animBits & Ty) {
                jointPtr->t.y = *jointframe++;
            }

            if (animBits & Tz) {
                jointPtr->t.z = *jointframe++;
            }
        }

        if (animBits & (Qx | Qy | Qz)) {
            if (animBits & Qx) {
                jointPtr->q.x = *jointframe++;
            }

            if (animBits & Qy) {
                jointPtr->q.y = *jointframe++;
            }

            if (animBits & Qz) {
                jointPtr->q.z = *jointframe++;
            }

            jointPtr->q.w = jointPtr->q.CalcW();
        }

        if (animBits & (Sx | Sy | Sz)) {
            if (animBits & Sx) {
                jointPtr->s.x = *jointframe++;
            }

            if (animBits & Sy) {
                jointPtr->s.y = *jointframe++;
            }

            if (animBits & Sz) {
                jointPtr->s.z = *jointframe++;
            }
        }
    }
    
#if CYCLIC_DELTA_MOVEMENT
    if (frame.cycleCount) {
        joints[0].t += totalDelta * (float)frame.cycleCount;
    }
#endif    
    
    if (!rootTranslationXY) {
        joints[0].t.x = baseFrame[0].t.x;
        joints[0].t.y = baseFrame[0].t.y;
    }

    if (!rootTranslationZ) {
        joints[0].t.z = baseFrame[0].t.z;
    }

    if (!rootRotation) {
        joints[0].q = baseFrame[0].q;
    }
}

void Anim::GetInterpolatedFrame(FrameInterpolation &frame, int numJointIndexes, const int *jointIndexes, JointPose *joints) const {
    // copy the baseframe
    simdProcessor->Memcpy(joints, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (!numAnimatedComponents) {
        // just use the base frame
        return;
    }

    JointPose *blendJoints = (JointPose *)_alloca16(baseFrame.Count() * sizeof(JointPose));
    int *lerpIndex = (int *)_alloca16(baseFrame.Count() * sizeof(lerpIndex[0]));
    int numLerpJoints = 0;

    const float *frame1 = &frameComponents[frame.frame1 * numAnimatedComponents];
    const float *frame2 = &frameComponents[frame.frame2 * numAnimatedComponents];

    for (int i = 0; i < numJointIndexes; i++) {
        int j = jointIndexes[i];
        const JointInfo *infoPtr = &jointInfo[j];

        int animBits = infoPtr->animBits;
        if (animBits == 0) {
            continue;
        }

        lerpIndex[numLerpJoints++] = j;

        JointPose *jointPtr = &joints[j];
        JointPose *blendPtr = &blendJoints[j];

        const float *jointframe1 = frame1 + infoPtr->firstComponent;
        const float *jointframe2 = frame2 + infoPtr->firstComponent;

        switch (animBits & (Tx | Ty | Tz)) {
        case 0:
            blendPtr->t = jointPtr->t;
            break;
        case Tx:
            jointPtr->t.x = jointframe1[0];
            blendPtr->t.x = jointframe2[0];
            blendPtr->t.y = jointPtr->t.y;
            blendPtr->t.z = jointPtr->t.z;
            jointframe1++;
            jointframe2++;
            break;
        case Ty:
            jointPtr->t.y = jointframe1[0];
            blendPtr->t.y = jointframe2[0];
            blendPtr->t.x = jointPtr->t.x;
            blendPtr->t.z = jointPtr->t.z;
            jointframe1++;
            jointframe2++;
            break;
        case Tz:
            jointPtr->t.z = jointframe1[0];
            blendPtr->t.z = jointframe2[0];
            blendPtr->t.x = jointPtr->t.x;
            blendPtr->t.y = jointPtr->t.y;
            jointframe1++;
            jointframe2++;
            break;
        case Tx | Ty:
            jointPtr->t.x = jointframe1[0];
            jointPtr->t.y = jointframe1[1];
            blendPtr->t.x = jointframe2[0];
            blendPtr->t.y = jointframe2[1];
            blendPtr->t.z = jointPtr->t.z;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Tx | Tz:
            jointPtr->t.x = jointframe1[0];
            jointPtr->t.z = jointframe1[1];
            blendPtr->t.x = jointframe2[0];
            blendPtr->t.z = jointframe2[1];
            blendPtr->t.y = jointPtr->t.y;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Ty | Tz:
            jointPtr->t.y = jointframe1[0];
            jointPtr->t.z = jointframe1[1];
            blendPtr->t.y = jointframe2[0];
            blendPtr->t.z = jointframe2[1];
            blendPtr->t.x = jointPtr->t.x;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Tx | Ty | Tz:
            jointPtr->t.x = jointframe1[0];
            jointPtr->t.y = jointframe1[1];
            jointPtr->t.z = jointframe1[2];
            blendPtr->t.x = jointframe2[0];
            blendPtr->t.y = jointframe2[1];
            blendPtr->t.z = jointframe2[2];
            jointframe1 += 3;
            jointframe2 += 3;
            break;
        }

        switch (animBits & (Qx | Qy | Qz)) {
        case 0:
            blendPtr->q = jointPtr->q;
            break;
        case Qx:
            jointPtr->q.x = jointframe1[0];
            blendPtr->q.x = jointframe2[0];
            blendPtr->q.y = jointPtr->q.y;
            blendPtr->q.z = jointPtr->q.z;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qy:
            jointPtr->q.y = jointframe1[0];
            blendPtr->q.y = jointframe2[0];
            blendPtr->q.x = jointPtr->q.x;
            blendPtr->q.z = jointPtr->q.z;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qz:
            jointPtr->q.z = jointframe1[0];
            blendPtr->q.z = jointframe2[0];
            blendPtr->q.x = jointPtr->q.x;
            blendPtr->q.y = jointPtr->q.y;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qx | Qy:
            jointPtr->q.x = jointframe1[0];
            jointPtr->q.y = jointframe1[1];
            blendPtr->q.x = jointframe2[0];
            blendPtr->q.y = jointframe2[1];
            blendPtr->q.z = jointPtr->q.z;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qx | Qz:
            jointPtr->q.x = jointframe1[0];
            jointPtr->q.z = jointframe1[1];
            blendPtr->q.x = jointframe2[0];
            blendPtr->q.z = jointframe2[1];
            blendPtr->q.y = jointPtr->q.y;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qy | Qz:
            jointPtr->q.y = jointframe1[0];
            jointPtr->q.z = jointframe1[1];
            blendPtr->q.y = jointframe2[0];
            blendPtr->q.z = jointframe2[1];
            blendPtr->q.x = jointPtr->q.x;
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        case Qx | Qy | Qz:
            jointPtr->q.x = jointframe1[0];
            jointPtr->q.y = jointframe1[1];
            jointPtr->q.z = jointframe1[2];
            blendPtr->q.x = jointframe2[0];
            blendPtr->q.y = jointframe2[1];
            blendPtr->q.z = jointframe2[2];
            jointPtr->q.w = jointPtr->q.CalcW();
            blendPtr->q.w = blendPtr->q.CalcW();
            break;
        }

        switch (animBits & (Sx | Sy | Sz)) {
        case 0:
            blendPtr->s = jointPtr->s;
            break;
        case Sx:
            jointPtr->s.x = jointframe1[0];
            blendPtr->s.x = jointframe2[0];
            blendPtr->s.y = jointPtr->s.y;
            blendPtr->s.z = jointPtr->s.z;
            jointframe1++;
            jointframe2++;
            break;
        case Sy:
            jointPtr->s.y = jointframe1[0];
            blendPtr->s.y = jointframe2[0];
            blendPtr->s.x = jointPtr->s.x;
            blendPtr->s.z = jointPtr->s.z;
            jointframe1++;
            jointframe2++;
            break;
        case Sz:
            jointPtr->s.z = jointframe1[0];
            blendPtr->s.z = jointframe2[0];
            blendPtr->s.x = jointPtr->s.x;
            blendPtr->s.y = jointPtr->s.y;
            jointframe1++;
            jointframe2++;
            break;
        case Sx | Sy:
            jointPtr->s.x = jointframe1[0];
            jointPtr->s.y = jointframe1[1];
            blendPtr->s.x = jointframe2[0];
            blendPtr->s.y = jointframe2[1];
            blendPtr->s.z = jointPtr->s.z;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Sx | Sz:
            jointPtr->s.x = jointframe1[0];
            jointPtr->s.z = jointframe1[1];
            blendPtr->s.x = jointframe2[0];
            blendPtr->s.z = jointframe2[1];
            blendPtr->s.y = jointPtr->s.y;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Sy | Sz:
            jointPtr->s.y = jointframe1[0];
            jointPtr->s.z = jointframe1[1];
            blendPtr->s.y = jointframe2[0];
            blendPtr->s.z = jointframe2[1];
            blendPtr->s.x = jointPtr->s.x;
            jointframe1 += 2;
            jointframe2 += 2;
            break;
        case Sx | Sy | Sz:
            jointPtr->s.x = jointframe1[0];
            jointPtr->s.y = jointframe1[1];
            jointPtr->s.z = jointframe1[2];
            blendPtr->s.x = jointframe2[0];
            blendPtr->s.y = jointframe2[1];
            blendPtr->s.z = jointframe2[2];
            jointframe1 += 3;
            jointframe2 += 3;
            break;
        }
    }

    simdProcessor->BlendJoints(joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints);

#if CYCLIC_DELTA_MOVEMENT
    if (frame.cycleCount) {
        joints[0].t += totalDelta * (float)frame.cycleCount;
    }
#endif    

    if (!rootTranslationXY) {
        joints[0].t.x = baseFrame[0].t.x;
        joints[0].t.y = baseFrame[0].t.y;
    }

    if (!rootTranslationZ) {
        joints[0].t.z = baseFrame[0].t.z;
    }

    if (!rootRotation) {
        joints[0].q = baseFrame[0].q;
    }
}

BE_NAMESPACE_END
