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
#include "Core/BinSearch.h"
#include "Render/Render.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Simd/Simd.h"

//#define CYCLIC_DELTA_MOVEMENT

BE_NAMESPACE_BEGIN

size_t Anim::Allocated() const {
    size_t size = joints.Allocated() + components.Allocated() + frameTimes.Allocated() + hashName.Allocated();
    return size;
}

void Anim::Purge() {
    numFrames = 0;
    numJoints = 0;
    length = 0;
    maxCycleCount = 1;

    rootRotation = false;
    rootTranslationXY = false;
    rootTranslationZ = false;

    totalDelta.SetFromScalar(0);
    
    joints.Clear();
    components.Clear();
    frameTimes.Clear();
}

Anim &Anim::Copy(const Anim &other) {
    numJoints = other.numJoints;
    numFrames = other.numFrames;
    numComponentsPerFrame = other.numComponentsPerFrame;
    length = other.length;
    maxCycleCount = other.maxCycleCount;

    rootRotation = other.rootRotation;
    rootTranslationXY = other.rootTranslationXY;
    rootTranslationZ = other.rootTranslationZ;

    joints = other.joints;
    baseFrame = other.baseFrame;
    components = other.components;
    frameTimes = other.frameTimes;
    totalDelta = other.totalDelta;

    return *this;
}

void Anim::CreateDefaultAnim(const Skeleton *skeleton) {
    isDefaultAnim = true;

    numJoints = skeleton->NumJoints();
    numFrames = 1;

    length = 1000;

    numComponentsPerFrame = 0;

    joints.SetGranularity(1);
    joints.SetCount(numJoints);

    const Joint *skeletonJoints = skeleton->GetJoints();

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        JointInfo *ji = &joints[jointIndex];

        ji->nameIndex = animManager.JointIndexByName(skeleton->GetJointName(jointIndex));

        if (skeletonJoints[jointIndex].parent) {
            ji->parentIndex = (int32_t)(skeletonJoints[jointIndex].parent - skeletonJoints);
        } else {
            ji->parentIndex = -1;
        }
        ji->componentBits = 0;
        ji->componentOffset = 0;
    }
    
    baseFrame.SetGranularity(1);
    baseFrame.SetCount(numJoints);

    for (int i = 0; i < numJoints; i++) {
        baseFrame[i] = skeleton->GetBindPoses()[i];
    }

    frameTimes.SetGranularity(1);
    frameTimes.SetCount(1);
    frameTimes[0] = 0;

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
            const JointInfo *jointInfoPtr = &joints[jointIndex];

            if (jointInfoPtr->componentBits == 0) {
                continue;
            }

            jointFrame[jointIndex] -= firstFrame[jointIndex];

            float *componentPtr = &additiveAnim->components[jointInfoPtr->componentOffset + i * numComponentsPerFrame];

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
        additiveAnim->baseFrame[jointIndex] -= firstFrame[jointIndex];
    }

    return additiveAnim;
}

Anim *Anim::CreateAdditiveAnim(const Skeleton *skeleton, int numJointIndexes, const int *jointIndexes) {
    if (NumJoints() != skeleton->NumJoints()) {
        BE_WARNLOG("different joint count for creating additive anim\n");
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
        BE_WARNLOG("different joint count for creating additive anim\n");
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

    BE_LOG("Loading anim '%s'...\n", bAnimFilename.c_str());

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
    // Checks if both have the same number of joints.
    if (joints.Count() != skeleton->NumJoints()) {
        BE_ERRLOG("Mesh '%s' has different number of joints than anim '%s'", skeleton->GetHashName(), hashName.c_str());
        return false;
    }

    const Joint *skeletonJoints = skeleton->GetJoints();

    for (int jointIndex = 0; jointIndex < joints.Count(); jointIndex++) {
        const JointInfo &joint = joints[jointIndex];
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
    if (!numComponentsPerFrame) {
        totalDelta.SetFromScalar(0);
    } else {
        const JointInfo &rootJoint = joints[0];

        const float *componentPtr = &components[rootJoint.componentOffset];

        if (rootJoint.componentBits & ComponentBit::Tx) {
            totalDelta.x = componentPtr[numComponentsPerFrame * (numFrames - 1)] - baseFrame[0].t[0];
            componentPtr++;
        } else {
            totalDelta.x = 0.0f;
        }

        if (rootJoint.componentBits & ComponentBit::Ty) {
            totalDelta.y = componentPtr[numComponentsPerFrame * (numFrames - 1)] - baseFrame[0].t[1];
            componentPtr++;
        } else {
            totalDelta.y = 0.0f;
        }

        if (rootJoint.componentBits & ComponentBit::Tz) {
            totalDelta.z = componentPtr[numComponentsPerFrame * (numFrames - 1)] - baseFrame[0].t[2];
        } else {
            totalDelta.z = 0.0f;
        }
    }

    BE_DLOG("animation '%s' total delta (%s)\n", name.c_str(), totalDelta.ToString(4));
}

void Anim::ComputeFrameAABBs(const Skeleton *skeleton, const Mesh *mesh, Array<AABB> &frameAABBs) const {
    if (skeleton->NumJoints() == 0) {
        return;
    }

    int *jointIndexes = (int *)_alloca16(numJoints * sizeof(int));
    int *jointParents = (int *)_alloca16(numJoints * sizeof(jointParents[0]));

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        jointIndexes[jointIndex] = jointIndex;
        jointParents[jointIndex] = joints[jointIndex].parentIndex;
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

        simdProcessor->MultiplyJoints(jointMats2, jointMats, skeleton->GetInvBindPoseMatrices(), numJoints);

        frameAABB.Clear();

        for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
            const SubMesh *subMesh = mesh->GetSurface(surfaceIndex)->subMesh;

            switch (subMesh->MaxVertexWeights()) {
            case 8: {
                const VertexWeight8 *vertWeights = (const VertexWeight8 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
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
                break; }
            case 4: {
                const VertexWeight4 *vertWeights = (const VertexWeight4 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
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
                break; }
            case 1: {
                const VertexWeight1 *vertWeights = (const VertexWeight1 *)subMesh->VertexWeights();

                for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                    const VertexWeight1 *vw = &vertWeights[vertexIndex];
                    const Vec3 &pos = subMesh->Verts()[vertexIndex].xyz;
                    Vec3 resultPos = jointMats2[vw->jointIndex].Transform(pos);

                    frameAABB.AddPoint(resultPos);
                }
                break; }
            }
        }
    }
}

void Anim::TimeToFrameInterpolation(int time, FrameInterpolation &frameInterpolation) const {
    if (numFrames <= 1) {
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

    int lastFrameTime = frameTimes.Last();

    frameInterpolation.cycleCount = time / lastFrameTime;

    if (maxCycleCount > 0 && frameInterpolation.cycleCount >= maxCycleCount) {
        frameInterpolation.cycleCount = maxCycleCount - 1;
        frameInterpolation.frame1 = numFrames - 1;
        frameInterpolation.frame2 = frameInterpolation.frame1;
        frameInterpolation.backlerp = 0.0f;
        frameInterpolation.frontlerp = 1.0f;
        return;
    }

    int t = time % lastFrameTime;

    int frameNum = BinSearch_LessEqual<int>(frameTimes.Ptr(), frameTimes.Count(), t);

    frameInterpolation.frame1 = frameNum;
    frameInterpolation.frame2 = frameInterpolation.frame1 + 1;

    int t1 = frameTimes[frameNum];
    int t2 = frameTimes[frameNum + 1];

    frameInterpolation.backlerp = (float)(t - t1) / (float)(t2 - t1);
    frameInterpolation.frontlerp = 1.0f - frameInterpolation.backlerp;
}

void Anim::GetTranslation(Vec3 &outTranslation, int time, bool isCyclicTranslation) const {
    //if (rootTranslationXY && rootTranslationZ) {
    //    outTranslation = baseFrame[0].t;
    //    return;
    //}

    FrameInterpolation frame;

    outTranslation[0] = baseFrame[0].t[0];
    outTranslation[1] = baseFrame[0].t[1];
    outTranslation[2] = baseFrame[0].t[2];

    const JointInfo &rootJoint = joints[0];

    if (!(rootJoint.componentBits & (ComponentBit::Tx | ComponentBit::Ty | ComponentBit::Tz))) {
        // just use the baseframe
        return;
    }

    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame2];

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

    const JointInfo &rootJoint = joints[0];

    int componentBits = rootJoint.componentBits;
    if (!(componentBits & (ComponentBit::Qx | ComponentBit::Qy | ComponentBit::Qz))) {
        // just use the baseframe
        outRotation[0] = baseFrame[0].q[0];
        outRotation[1] = baseFrame[0].q[1];
        outRotation[2] = baseFrame[0].q[2];
        outRotation[3] = baseFrame[0].q[3];
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame2];

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

    Quat q1;
    Quat q2;

    switch (componentBits & (ComponentBit::Qx | ComponentBit::Qy | ComponentBit::Qz)) {
    case ComponentBit::Qx:
        q1.x = componentPtr1[0];
        q2.x = componentPtr2[0];
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qy:
        q1.y = componentPtr1[0];
        q2.y = componentPtr2[0];
        q1.x = baseFrame[0].q[0];
        q2.x = q1.x;
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qz:
        q1.z = componentPtr1[0];
        q2.z = componentPtr2[0];
        q1.x = baseFrame[0].q[0];
        q2.x = q1.x;
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qx | ComponentBit::Qy:
        q1.x = componentPtr1[0];
        q1.y = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.y = componentPtr2[1];
        q1.z = baseFrame[0].q[2];
        q2.z = q1.z;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qx | ComponentBit::Qz:
        q1.x = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.x = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.y = baseFrame[0].q[1];
        q2.y = q1.y;
        q1.w = q1.CalcW();
        q2.w = q2.CalcW();
        break;
    case ComponentBit::Qy | ComponentBit::Qz:
        q1.y = componentPtr1[0];
        q1.z = componentPtr1[1];
        q2.y = componentPtr2[0];
        q2.z = componentPtr2[1];
        q1.x = baseFrame[0].q[0];
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

void Anim::GetScaling(Vec3 &outScaling, int time) const {
    const JointInfo &rootJoint = joints[0];

    int componentBits = rootJoint.componentBits;
    if (!(componentBits & (ComponentBit::Sx | ComponentBit::Sy | ComponentBit::Sz))) {
        // just use the baseframe
        outScaling = baseFrame[0].s;
        return;
    }

    FrameInterpolation frame;
    TimeToFrameInterpolation(time, frame);

    const float *componentPtr1 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame1];
    const float *componentPtr2 = &components[rootJoint.componentOffset + numComponentsPerFrame * frame.frame2];

    if (componentBits & (ComponentBit::Tx | ComponentBit::Ty | ComponentBit::Tz)) {
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

    if (componentBits & (ComponentBit::Qx | ComponentBit::Qy | ComponentBit::Qz)) {
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

void Anim::GetAABB(AABB &outAabb, const Array<AABB> &frameAABBs, int time) const {
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
        Vec3 delta = totalDelta * (float)frame.cycleCount;
        outAabb[0] += delta;
        outAabb[1] += delta;
    }
    bool cyclicTranslation = true;
#else
    bool cyclicTranslation = false;
#endif
}

static void DecodeSingleFrame(const Anim::JointInfo *joints, int numJointIndexes, const int *jointIndexes, const float *frameComponents, JointPose *frame) {
    for (int i = 0; i < numJointIndexes; i++) {
        int jointIndex = jointIndexes[i];
        const Anim::JointInfo *jointInfoPtr = &joints[jointIndex];

        int componentBits = jointInfoPtr->componentBits;
        if (componentBits == 0) {
            continue;
        }

        JointPose *frameJointPtr = &frame[jointIndex];
        const float *jointComponentPtr = frameComponents + jointInfoPtr->componentOffset;

        if (componentBits & (Anim::ComponentBit::Tx | Anim::ComponentBit::Ty | Anim::ComponentBit::Tz)) {
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

        if (componentBits & (Anim::ComponentBit::Qx | Anim::ComponentBit::Qy | Anim::ComponentBit::Qz)) {
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

        if (componentBits & (Anim::ComponentBit::Sx | Anim::ComponentBit::Sy | Anim::ComponentBit::Sz)) {
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

void Anim::GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, JointPose *frame) const {
    // Copy the base frame
    simdProcessor->Memcpy(frame, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (frameNum == 0 || !numComponentsPerFrame) {
        // Just use the base frame
        return;
    }

    const float *frameComponents = &components[frameNum * numComponentsPerFrame];

    DecodeSingleFrame(joints.Ptr(), numJointIndexes, jointIndexes, frameComponents, frame);

    if (!rootTranslationXY) {
        frame[0].t.x = baseFrame[0].t.x;
        frame[0].t.y = baseFrame[0].t.y;
    }

    if (!rootTranslationZ) {
        frame[0].t.z = baseFrame[0].t.z;
    }

    if (!rootRotation) {
        frame[0].q = baseFrame[0].q;
    }
}

static int DecodeInterpolatedFrame(const Anim::JointInfo *joints, int numJointIndexes, const int *jointIndexes, const float *frameComponents1, const float *frameComponents2,
    JointPose *frame, JointPose *blendFrame, int *lerpIndex) {
    int numLerpJoints = 0;

    for (int i = 0; i < numJointIndexes; i++) {
        int jointIndex = jointIndexes[i];
        const Anim::JointInfo *jointInfoPtr = &joints[jointIndex];

        int componentBits = jointInfoPtr->componentBits;
        if (componentBits == 0) {
            continue;
        }

        lerpIndex[numLerpJoints++] = jointIndex;

        JointPose *frameJointPtr = &frame[jointIndex];
        JointPose *blendJointPtr = &blendFrame[jointIndex];

        const float *jointComponentPtr1 = frameComponents1 + jointInfoPtr->componentOffset;
        const float *jointComponentPtr2 = frameComponents2 + jointInfoPtr->componentOffset;

        switch (componentBits & (Anim::ComponentBit::Tx | Anim::ComponentBit::Ty | Anim::ComponentBit::Tz)) {
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

        switch (componentBits & (Anim::ComponentBit::Qx | Anim::ComponentBit::Qy | Anim::ComponentBit::Qz)) {
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

        switch (componentBits & (Anim::ComponentBit::Sx | Anim::ComponentBit::Sy | Anim::ComponentBit::Sz)) {
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

void Anim::GetInterpolatedFrame(FrameInterpolation &frameInterpolation, int numJointIndexes, const int *jointIndexes, JointPose *frame) const {
    // Copy the base frame
    simdProcessor->Memcpy(frame, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (!numComponentsPerFrame) {
        // Just use the base frame
        return;
    }

    JointPose *blendFrame = (JointPose *)_alloca16(baseFrame.Count() * sizeof(JointPose));
    int *lerpIndex = (int *)_alloca16(baseFrame.Count() * sizeof(lerpIndex[0]));

    const float *frameComponents1 = &components[frameInterpolation.frame1 * numComponentsPerFrame];
    const float *frameComponents2 = &components[frameInterpolation.frame2 * numComponentsPerFrame];

    int numLerpJoints = DecodeInterpolatedFrame(joints.Ptr(), numJointIndexes, jointIndexes, frameComponents1, frameComponents2, frame, blendFrame, lerpIndex);

    simdProcessor->BlendJoints(frame, blendFrame, frameInterpolation.backlerp, lerpIndex, numLerpJoints);

#if CYCLIC_DELTA_MOVEMENT
    if (frameInterpolation.cycleCount) {
        frame[0].t += totalDelta * (float)frameInterpolation.cycleCount;
    }
#endif

    if (!rootTranslationXY) {
        frame[0].t.x = baseFrame[0].t.x;
        frame[0].t.y = baseFrame[0].t.y;
    }

    if (!rootTranslationZ) {
        frame[0].t.z = baseFrame[0].t.z;
    }

    if (!rootRotation) {
        frame[0].q = baseFrame[0].q;
    }
}

BE_NAMESPACE_END
