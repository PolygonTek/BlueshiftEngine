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
#include "Core/Vertex.h"
#include "Core/JointPose.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

#define UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2) {Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);} }
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4) {Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);} for (;_IX<count;_IX++){Y(_IX);} }
#define UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8) {Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for (;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }

void BE_FASTCALL SIMD_Generic::Add(float *dst, const float constant, const float *src, const int count) {
#define OPER(X) dst[(X)] = constant + src[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Add(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] + src1[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Sub(float *dst, const float constant, const float *src, const int count) {
#define OPER(X) dst[(X)] = constant - src[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Sub(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] - src1[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Mul(float *dst, const float constant, const float *src, const int count) {
#define OPER(X) dst[(X)] = constant * src[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Mul(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] * src1[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Div(float *dst, const float constant, const float *src, const int count) {
#define OPER(X) dst[(X)] = constant / src[(X)];
    UNROLL4(OPER)
#undef OPER
}

void BE_FASTCALL SIMD_Generic::Div(float *dst, const float *src0, const float *src1, const int count) {
#define OPER(X) dst[(X)] = src0[(X)] / src1[(X)];
    UNROLL4(OPER)
#undef OPER
}

float BE_FASTCALL SIMD_Generic::Sum(const float *src, const int count) {
    float ret = 0;

    for (int i = 0; i < count; i++) {
        ret += src[i];
    }

    return ret;
}

void BE_FASTCALL SIMD_Generic::Memcpy(void *dst, const void *src, const int count) {
    memcpy(dst, src, count);
}

void BE_FASTCALL SIMD_Generic::Memset(void *dst, const int val, const int count) {
    memset(dst, val, count);
}

void BE_FASTCALL SIMD_Generic::DecompressJoints(JointPose *joints, const CompressedJointPose *compressedJoints, const int *index, const int numJoints) {
    for (int i = 0; i < numJoints; i++) {
        int j = index[i];

        joints[j].q = compressedJoints[j].ToQuat();
        joints[j].t = compressedJoints[j].ToTranslation();
        joints[j].s = compressedJoints[j].ToScale();
    }
}

void BE_FASTCALL SIMD_Generic::BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    int i, j;

    for (i = 0; i < numJoints; i++) {
        j = index[i];
        joints[j].q.SetFromSlerp(joints[j].q, blendJoints[j].q, fraction);
        joints[j].t.SetFromLerp(joints[j].t, blendJoints[j].t, fraction);
        joints[j].s.SetFromLerp(joints[j].s, blendJoints[j].s, fraction);
    }
}

void BE_FASTCALL SIMD_Generic::BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    int i, j;

    for (i = 0; i < numJoints; i++) {
        j = index[i];
        joints[j].q.SetFromSlerpFast(joints[j].q, blendJoints[j].q, fraction);
        joints[j].t.SetFromLerp(joints[j].t, blendJoints[j].t, fraction);
        joints[j].s.SetFromLerp(joints[j].s, blendJoints[j].s, fraction);
    }
}

void BE_FASTCALL SIMD_Generic::AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) {
    int i, j;

    for (i = 0; i < numJoints; i++) {
        j = index[i];
        joints[j].q.SetFromSlerp(joints[j].q, blendJoints[j].q * joints[j].q, fraction);
        joints[j].t += blendJoints[j].t * fraction;
        Vec3 &jointScale = joints[j].s;
        const Vec3 &blendScale = blendJoints[j].s;
        jointScale.x *= 1.0f + (blendScale.x - 1.0f) * fraction;
        jointScale.y *= 1.0f + (blendScale.y - 1.0f) * fraction;
        jointScale.z *= 1.0f + (blendScale.z - 1.0f) * fraction;
    }
}

void BE_FASTCALL SIMD_Generic::ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) {
    int i;

    for (i = 0; i < numJoints; i++) {
        jointMats[i].SetTQS(jointPoses[i].t, jointPoses[i].q, jointPoses[i].s);
    }
}

void BE_FASTCALL SIMD_Generic::ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints) {
    int i;

    for (i = 0; i < numJoints; i++) {
        jointPoses[i].SetFromMat3x4(jointMats[i]);
    }
}

void BE_FASTCALL SIMD_Generic::TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    int i;

    for (i = firstJoint; i <= lastJoint; i++) {
        assert(parents[i] < i);
        if (parents[i] >= 0) {
            jointMats[i].TransformSelf(jointMats[parents[i]]);
        }
    }
}

void BE_FASTCALL SIMD_Generic::TransformJoints(const Mat3x4 *localJointMats, Mat3x4 *worldJointMats, const int *parents, const int firstJoint, const int lastJoint) {
    int i;

    for (i = firstJoint; i <= lastJoint; i++) {
        const int parentIndex = parents[i];
        assert(parentIndex < i);

        if (parentIndex < 0) {
            worldJointMats[i] = localJointMats[i];
        } else {
            worldJointMats[i] = worldJointMats[parentIndex] * localJointMats[i];
        }
    }
}

void BE_FASTCALL SIMD_Generic::UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    int i;

    for (i = lastJoint; i >= firstJoint; i--) {
        assert(parents[i] < i);
        if (parents[i] >= 0) {
            jointMats[i].UntransformSelf(jointMats[parents[i]]);
        }
    }
}

void BE_FASTCALL SIMD_Generic::MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) {
    int i;

    for (i = 0; i < numJoints; i++) {
        result[i] = joints1[i] * joints2[i];
    }
}

BE_NAMESPACE_END
