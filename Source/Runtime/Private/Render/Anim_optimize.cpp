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

BE_NAMESPACE_BEGIN

static float CompareJointsT(const JointPose *joints1, const JointPose *joints2, int numJoints) {
    float maxDeltaT = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const JointPose *joint1Ptr = &joints1[i];
        const JointPose *joint2Ptr = &joints2[i];

        maxDeltaT = Max(maxDeltaT, joint1Ptr->t.DistanceSqr(joint2Ptr->t));
    }

    return maxDeltaT;
}

static float CompareJointsQ(const JointPose *joints1, const JointPose *joints2, int numJoints) {
    float maxDeltaQ = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const JointPose *joint1Ptr = &joints1[i];
        const JointPose *joint2Ptr = &joints2[i];

        // NOTE: w 값을 양수로 유지하기 위해 -1 을 곱한 xyz 를 고려해야 한다. 오차값이 실제보다 더 작게 나올 수도 있지만, 일단은 이렇게..
        maxDeltaQ = Max(maxDeltaQ, Math::Fabs(Math::Fabs(joint1Ptr->q.x) - Math::Fabs(joint2Ptr->q.x)));
        maxDeltaQ = Max(maxDeltaQ, Math::Fabs(Math::Fabs(joint1Ptr->q.y) - Math::Fabs(joint2Ptr->q.y)));
        maxDeltaQ = Max(maxDeltaQ, Math::Fabs(Math::Fabs(joint1Ptr->q.z) - Math::Fabs(joint2Ptr->q.z)));
    }

    return maxDeltaQ;
}

static float CompareJointsS(const JointPose *joints1, const JointPose *joints2, int numJoints) {
    float maxDeltaS = 0.0f;

    for (int i = 0; i < numJoints; i++) {
        const JointPose *joint1Ptr = &joints1[i];
        const JointPose *joint2Ptr = &joints2[i];

        // FIXME: compare method
        maxDeltaS = Max(maxDeltaS, joint1Ptr->s.DistanceSqr(joint2Ptr->s));
    }

    return maxDeltaS;
}

void Anim::LerpFrame(int framenum1, int framenum2, float backlerp, JointPose *joints) {
    assert(0 <= framenum1 && framenum1 < numFrames);
    assert(0 <= framenum2 && framenum2 < numFrames);

    // copy the baseframe
    simdProcessor->Memcpy(joints, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (framenum1 == framenum2) {
        return;
    }

    int *lerpIndex = (int *)_alloca16(numJoints * sizeof(lerpIndex[0]));
    int numLerpJoints = 0;

    JointPose *jointFrame = (JointPose *)_alloca16(numJoints * sizeof(JointPose));
    JointPose *blendJoints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));

    const float *frame1 = &frameComponents[framenum1 * numAnimatedComponents];
    const float *frame2 = &frameComponents[framenum2 * numAnimatedComponents];

    for (int i = 0; i < numJoints; i++) {
        const Anim::JointInfo *infoPtr = &jointInfo[i];

        int animBits = infoPtr->animBits;
        if (animBits == 0) {
            continue;
        }

        lerpIndex[numLerpJoints++] = i;

        JointPose *jointPtr = &joints[i];
        JointPose *blendPtr = &blendJoints[i];

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

    simdProcessor->BlendJoints(joints, blendJoints, backlerp, lerpIndex, numLerpJoints);
}

void Anim::RemoveFrames(int numRemoveFrames, const int *removeFramenums) {
    Array<float>	newFrameComponents;
    Array<int>	newFrameTimes;
    Array<AABB>	newAABBs;

    newFrameComponents.SetGranularity(1);
    newFrameComponents.SetCount(frameComponents.Count() - numRemoveFrames * numAnimatedComponents);
    newFrameTimes.SetGranularity(1);
    newFrameTimes.SetCount(frameToTimeMap.Count() - numRemoveFrames);
    newAABBs.SetGranularity(1);

    int numNewFrames = 0;

    for (int frameNum = 0; frameNum < numFrames; frameNum++) {
        bool remove = false;
        for (int i = 0; i < numRemoveFrames; i++) {
            if (frameNum == removeFramenums[i]) {
                remove = true;
                break;
            }
        }

        if (remove) {
            continue;
        }

        for (int i = 0; i < numAnimatedComponents; i++) {
            newFrameComponents[numNewFrames * numAnimatedComponents + i] = frameComponents[frameNum * numAnimatedComponents + i];
        }
        newFrameTimes[numNewFrames] = frameToTimeMap[frameNum];

        numNewFrames++;
    }

    frameComponents = newFrameComponents;
    frameToTimeMap = newFrameTimes;

    numFrames = numNewFrames;
}

void Anim::OptimizeFrames(float epsilonT, float epsilonQ, float epsilonS) {
    if (!numAnimatedComponents) {
        return;
    }

    Array<int> removeFrameNums;
    removeFrameNums.Resize(numFrames);

    JointPose *joints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));
    JointPose *lerpedJoints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));
    
    int *jointIndexes = (int *)_alloca16(numJoints * sizeof(int));
    for (int i = 0; i < numJoints; i++) {
        jointIndexes[i] = i;
    }

    int framenum1 = 0;
    int framenum2 = numFrames - 1;

    while (framenum1 != framenum2) {
        if (framenum2 - framenum1 <= 1) {
            framenum1 = framenum2;
            framenum2 = numFrames - 1;
            continue;
        }

        float blockTime = frameToTimeMap[framenum2] - frameToTimeMap[framenum1];
        float maxdt = 0.0f;
        float maxdq = 0.0f;
        float maxds = 0.0f;
        int framenum3 = framenum1;
        int framenum4 = framenum1;
        int framenum5 = framenum1;

        for (int i = framenum1 + 1; i < framenum2; i++) {
            GetSingleFrame(i, numJoints, jointIndexes, joints);

            float backlerp = (frameToTimeMap[i] - frameToTimeMap[framenum1]) / blockTime;
            LerpFrame(framenum1, framenum2, backlerp, lerpedJoints);

            float dt = CompareJointsT(joints, lerpedJoints, numJoints);
            if (dt > maxdt) {
                maxdt = dt;
                framenum3 = i;
            }

            float dq = CompareJointsQ(joints, lerpedJoints, numJoints);
            if (dq > maxdq) {
                maxdq = dq;
                framenum4 = i;
            }

            float ds = CompareJointsS(joints, lerpedJoints, numJoints);
            if (ds > maxds) {
                maxds = ds;
                framenum5 = i;
            }
        }
    
        if (maxdt > epsilonT || maxdq > epsilonQ || maxds > epsilonS) {	
            framenum2 = Max3(framenum3, framenum4, framenum5);
            continue;
        }

        for (int i = framenum1 + 1; i < framenum2; i++) {
            removeFrameNums.Append(i);
        }

        framenum1 = framenum2;
        framenum2 = numFrames - 1;
    }

    if (removeFrameNums.Count() > 0) {
        BE_LOG(L"%.1f%% of frames removed\n", 100.0f * removeFrameNums.Count() / numFrames);
    }

    RemoveFrames(removeFrameNums.Count(), removeFrameNums.Ptr());
}

BE_NAMESPACE_END
