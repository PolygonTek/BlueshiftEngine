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

void Anim::LerpFrame(int frameNum1, int frameNum2, float backlerp, JointPose *frame) {
    assert(0 <= frameNum1 && frameNum1 < numFrames);
    assert(0 <= frameNum2 && frameNum2 < numFrames);

    // copy the baseframe
    simdProcessor->Memcpy(frame, baseFrame.Ptr(), baseFrame.Count() * sizeof(baseFrame[0]));

    if (frameNum1 == frameNum2) {
        return;
    }

    int *lerpIndex = (int *)_alloca16(numJoints * sizeof(lerpIndex[0]));
    int numLerpJoints = 0;

    JointPose *blendFrame = (JointPose *)_alloca16(numJoints * sizeof(JointPose));

    const float *frameComponents1 = &components[frameNum1 * numComponentsPerFrame];
    const float *frameComponents2 = &components[frameNum2 * numComponentsPerFrame];

    for (int i = 0; i < numJoints; i++) {
        const Anim::JointInfo *jointInfoPtr = &joints[i];

        int componentBits = jointInfoPtr->componentBits;
        if (componentBits == 0) {
            continue;
        }

        lerpIndex[numLerpJoints++] = i;

        JointPose *frameJointPtr = &frame[i];
        JointPose *blendJointPtr = &blendFrame[i];

        const float *jointComponentPtr1 = frameComponents1 + jointInfoPtr->componentOffset;
        const float *jointComponentPtr2 = frameComponents2 + jointInfoPtr->componentOffset;

        switch (componentBits & (Tx | Ty | Tz)) {
        case 0:
            blendJointPtr->t = frameJointPtr->t;
            break;
        case Tx:
            frameJointPtr->t.x = jointComponentPtr1[0];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.y = frameJointPtr->t.y;
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Ty:
            frameJointPtr->t.y = jointComponentPtr1[0];
            blendJointPtr->t.y = jointComponentPtr2[0];
            blendJointPtr->t.x = frameJointPtr->t.x;
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Tz:
            frameJointPtr->t.z = jointComponentPtr1[0];
            blendJointPtr->t.z = jointComponentPtr2[0];
            blendJointPtr->t.x = frameJointPtr->t.x;
            blendJointPtr->t.y = frameJointPtr->t.y;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Tx | Ty:
            frameJointPtr->t.x = jointComponentPtr1[0];
            frameJointPtr->t.y = jointComponentPtr1[1];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.y = jointComponentPtr2[1];
            blendJointPtr->t.z = frameJointPtr->t.z;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Tx | Tz:
            frameJointPtr->t.x = jointComponentPtr1[0];
            frameJointPtr->t.z = jointComponentPtr1[1];
            blendJointPtr->t.x = jointComponentPtr2[0];
            blendJointPtr->t.z = jointComponentPtr2[1];
            blendJointPtr->t.y = frameJointPtr->t.y;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Ty | Tz:
            frameJointPtr->t.y = jointComponentPtr1[0];
            frameJointPtr->t.z = jointComponentPtr1[1];
            blendJointPtr->t.y = jointComponentPtr2[0];
            blendJointPtr->t.z = jointComponentPtr2[1];
            blendJointPtr->t.x = frameJointPtr->t.x;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Tx | Ty | Tz:
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

        switch (componentBits & (Qx | Qy | Qz)) {
        case 0:
            blendJointPtr->q = frameJointPtr->q;
            break;
        case Qx:
            frameJointPtr->q.x = jointComponentPtr1[0];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.y = frameJointPtr->q.y;
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qy:
            frameJointPtr->q.y = jointComponentPtr1[0];
            blendJointPtr->q.y = jointComponentPtr2[0];
            blendJointPtr->q.x = frameJointPtr->q.x;
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qz:
            frameJointPtr->q.z = jointComponentPtr1[0];
            blendJointPtr->q.z = jointComponentPtr2[0];
            blendJointPtr->q.x = frameJointPtr->q.x;
            blendJointPtr->q.y = frameJointPtr->q.y;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qx | Qy:
            frameJointPtr->q.x = jointComponentPtr1[0];
            frameJointPtr->q.y = jointComponentPtr1[1];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.y = jointComponentPtr2[1];
            blendJointPtr->q.z = frameJointPtr->q.z;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qx | Qz:
            frameJointPtr->q.x = jointComponentPtr1[0];
            frameJointPtr->q.z = jointComponentPtr1[1];
            blendJointPtr->q.x = jointComponentPtr2[0];
            blendJointPtr->q.z = jointComponentPtr2[1];
            blendJointPtr->q.y = frameJointPtr->q.y;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qy | Qz:
            frameJointPtr->q.y = jointComponentPtr1[0];
            frameJointPtr->q.z = jointComponentPtr1[1];
            blendJointPtr->q.y = jointComponentPtr2[0];
            blendJointPtr->q.z = jointComponentPtr2[1];
            blendJointPtr->q.x = frameJointPtr->q.x;
            frameJointPtr->q.w = frameJointPtr->q.CalcW();
            blendJointPtr->q.w = blendJointPtr->q.CalcW();
            break;
        case Qx | Qy | Qz:
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

        switch (componentBits & (Sx | Sy | Sz)) {
        case 0:
            blendJointPtr->s = frameJointPtr->s;
            break;
        case Sx:
            frameJointPtr->s.x = jointComponentPtr1[0];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.y = frameJointPtr->s.y;
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Sy:
            frameJointPtr->s.y = jointComponentPtr1[0];
            blendJointPtr->s.y = jointComponentPtr2[0];
            blendJointPtr->s.x = frameJointPtr->s.x;
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Sz:
            frameJointPtr->s.z = jointComponentPtr1[0];
            blendJointPtr->s.z = jointComponentPtr2[0];
            blendJointPtr->s.x = frameJointPtr->s.x;
            blendJointPtr->s.y = frameJointPtr->s.y;
            jointComponentPtr1++;
            jointComponentPtr2++;
            break;
        case Sx | Sy:
            frameJointPtr->s.x = jointComponentPtr1[0];
            frameJointPtr->s.y = jointComponentPtr1[1];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.y = jointComponentPtr2[1];
            blendJointPtr->s.z = frameJointPtr->s.z;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Sx | Sz:
            frameJointPtr->s.x = jointComponentPtr1[0];
            frameJointPtr->s.z = jointComponentPtr1[1];
            blendJointPtr->s.x = jointComponentPtr2[0];
            blendJointPtr->s.z = jointComponentPtr2[1];
            blendJointPtr->s.y = frameJointPtr->s.y;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Sy | Sz:
            frameJointPtr->s.y = jointComponentPtr1[0];
            frameJointPtr->s.z = jointComponentPtr1[1];
            blendJointPtr->s.y = jointComponentPtr2[0];
            blendJointPtr->s.z = jointComponentPtr2[1];
            blendJointPtr->s.x = frameJointPtr->s.x;
            jointComponentPtr1 += 2;
            jointComponentPtr2 += 2;
            break;
        case Sx | Sy | Sz:
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

    simdProcessor->BlendJoints(frame, blendFrame, backlerp, lerpIndex, numLerpJoints);
}

void Anim::RemoveFrames(int numRemoveFrames, const int *removeFrameNums) {
    Array<float> newFrameComponents;
    newFrameComponents.SetGranularity(1);
    newFrameComponents.SetCount(components.Count() - numRemoveFrames * numComponentsPerFrame);

    Array<int> newFrameTimes;
    newFrameTimes.SetGranularity(1);
    newFrameTimes.SetCount(frameTimes.Count() - numRemoveFrames);

    Array<AABB> newAABBs;
    newAABBs.SetGranularity(1);

    int numNewFrames = 0;

    for (int frameNum = 0; frameNum < numFrames; frameNum++) {
        bool remove = false;
        for (int i = 0; i < numRemoveFrames; i++) {
            if (frameNum == removeFrameNums[i]) {
                remove = true;
                break;
            }
        }

        if (remove) {
            continue;
        }

        for (int i = 0; i < numComponentsPerFrame; i++) {
            newFrameComponents[numNewFrames * numComponentsPerFrame + i] = components[frameNum * numComponentsPerFrame + i];
        }
        newFrameTimes[numNewFrames] = frameTimes[frameNum];

        numNewFrames++;
    }

    components = newFrameComponents;
    frameTimes = newFrameTimes;

    numFrames = numNewFrames;
}

void Anim::OptimizeFrames(float epsilonT, float epsilonQ, float epsilonS) {
    if (!numComponentsPerFrame) {
        return;
    }

    Array<int> removeFrameNums;
    removeFrameNums.Reserve(numFrames);

    JointPose *joints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));
    JointPose *lerpedJoints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));
    
    int *jointIndexes = (int *)_alloca16(numJoints * sizeof(int));
    for (int i = 0; i < numJoints; i++) {
        jointIndexes[i] = i;
    }

    int frameNum1 = 0;
    int frameNum2 = numFrames - 1;

    while (frameNum1 < frameNum2) {
        if (frameNum2 - frameNum1 <= 1) {
            frameNum1 = frameNum2;
            frameNum2 = numFrames - 1;
            continue;
        }

        float blockTime = frameTimes[frameNum2] - frameTimes[frameNum1];
        float maxDt = 0.0f;
        float maxDq = 0.0f;
        float maxDs = 0.0f;
        int frameNumMaxDt = frameNum1;
        int frameNumMaxDq = frameNum1;
        int frameNumMaxDs = frameNum1;

        for (int i = frameNum1 + 1; i < frameNum2; i++) {
            // Get the reference joints.
            GetSingleFrame(i, numJoints, jointIndexes, joints);

            // Get the interpolated joints.
            float backlerp = (frameTimes[i] - frameTimes[frameNum1]) / blockTime;
            LerpFrame(frameNum1, frameNum2, backlerp, lerpedJoints);

            float dt = CompareJointsT(joints, lerpedJoints, numJoints);
            if (dt > maxDt) {
                maxDt = dt;
                frameNumMaxDt = i;
            }

            float dq = CompareJointsQ(joints, lerpedJoints, numJoints);
            if (dq > maxDq) {
                maxDq = dq;
                frameNumMaxDq = i;
            }

            float ds = CompareJointsS(joints, lerpedJoints, numJoints);
            if (ds > maxDs) {
                maxDs = ds;
                frameNumMaxDs = i;
            }
        }
    
        if (maxDt > epsilonT || maxDq > epsilonQ || maxDs > epsilonS) {
            frameNum2 = Max3(frameNumMaxDt, frameNumMaxDq, frameNumMaxDs);
            continue;
        }

        for (int i = frameNum1 + 1; i < frameNum2; i++) {
            removeFrameNums.Append(i);
        }

        frameNum1 = frameNum2;
        frameNum2 = numFrames - 1;
    }

    if (removeFrameNums.Count() > 0) {
        BE_LOG("%.1f%% of frames removed\n", 100.0f * removeFrameNums.Count() / numFrames);
    }

    RemoveFrames(removeFrameNums.Count(), removeFrameNums.Ptr());
}

BE_NAMESPACE_END
