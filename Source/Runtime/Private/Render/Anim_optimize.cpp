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
#include "Core/Heap.h"
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

void Anim::RemoveFrames(int numRemoveFrames, const int *removableFrameNums) {
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
            if (frameNum == removableFrameNums[i]) {
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

void Anim::ComputeRemovableFrames(const JointPose *frameJoints, const int *jointIndexes, JointPose *lerpedJoints,
    float epsilonT, float epsilonQ, float epsilonS, int frameNum1, int frameNum2, Array<int> &removableFrameNums) {
    if (frameNum1 + 1 == frameNum2) {
        return;
    }

    float blockTime = frameTimes[frameNum2] - frameTimes[frameNum1];

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
        frameInterpolation.backlerp = (frameTimes[frameNum] - frameTimes[frameNum1]) / blockTime;
        frameInterpolation.frontlerp = 1.0f - frameInterpolation.backlerp;
        GetInterpolatedFrame(frameInterpolation, numJoints, jointIndexes, lerpedJoints);

        float dt = CompareJointsT(&frameJoints[frameNum * numJoints], lerpedJoints, numJoints);
        if (dt > maxDt) {
            maxDt = dt;
            frameNumMaxDt = frameNum;
        }

        float dq = CompareJointsQ(&frameJoints[frameNum * numJoints], lerpedJoints, numJoints);
        if (dq > maxDq) {
            maxDq = dq;
            frameNumMaxDq = frameNum;
        }

        float ds = CompareJointsS(&frameJoints[frameNum * numJoints], lerpedJoints, numJoints);
        if (ds > maxDs) {
            maxDs = ds;
            frameNumMaxDs = frameNum;
        }
    }

    if (maxDt > epsilonT || maxDq > epsilonQ || maxDs > epsilonS) {
        int frameNum = Max3(frameNumMaxDt, frameNumMaxDq, frameNumMaxDs);

        ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum1, frameNum, removableFrameNums);
        ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum, frameNum2, removableFrameNums);
        return;
    }

    for (int i = frameNum1 + 1; i < frameNum2; i++) {
        removableFrameNums.Append(i);
    }
}

void Anim::OptimizeFrames(float epsilonT, float epsilonQ, float epsilonS) {
    if (numFrames <= 2 || !numComponentsPerFrame) {
        return;
    }
    
    // Set up whole joint indexes
    int *jointIndexes = (int *)_alloca16(numJoints * sizeof(int));
    for (int i = 0; i < numJoints; i++) {
        jointIndexes[i] = i;
    }

    JointPose *frameJoints = (JointPose *)Mem_Alloc16(numFrames * numJoints * sizeof(JointPose));

    // Get the reference joints.
    for (int frameNum = 0; frameNum < numFrames; frameNum++) {
        GetSingleFrame(frameNum, numJoints, jointIndexes, &frameJoints[frameNum * numJoints]);
    }

    JointPose *lerpedJoints = (JointPose *)_alloca16(numJoints * sizeof(JointPose));

    int frameNum1 = 0;
    int frameNum2 = numFrames - 1;

    Array<int> removableFrameNums;
    removableFrameNums.Reserve(numFrames);

    ComputeRemovableFrames(frameJoints, jointIndexes, lerpedJoints, epsilonT, epsilonQ, epsilonS, frameNum1, frameNum2, removableFrameNums);

    if (removableFrameNums.Count() > 0) {
        BE_LOG("%.1f%% of frames removed\n", 100.0f * removableFrameNums.Count() / numFrames);
    }

    RemoveFrames(removableFrameNums.Count(), removableFrameNums.Ptr());

    Mem_AlignedFree(frameJoints);
}

BE_NAMESPACE_END
