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
#include "Core/Vertex.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Simd/Simd_Generic.h"

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

void BE_FASTCALL SIMD_Generic::MatrixTranspose(float *dst, const float *src) {
    dst[0] = src[0];
    dst[1] = src[4];
    dst[2] = src[8];
    dst[3] = src[12];

    dst[4] = src[1];
    dst[5] = src[5];
    dst[6] = src[9];
    dst[7] = src[13];
    
    dst[8] = src[2];
    dst[9] = src[6];
    dst[10] = src[10];
    dst[11] = src[14];

    dst[12] = src[3];
    dst[13] = src[7];
    dst[14] = src[11];
    dst[15] = src[15];
}

void BE_FASTCALL SIMD_Generic::MatrixMultiply(float *dst, const float *src0, const float *src1) {
    // row 0
    dst[0] = src0[0] * src1[0] + src0[1] * src1[4] + src0[2] * src1[8] + src0[3] * src1[12];
    dst[1] = src0[0] * src1[1] + src0[1] * src1[5] + src0[2] * src1[9] + src0[3] * src1[13];
    dst[2] = src0[0] * src1[2] + src0[1] * src1[6] + src0[2] * src1[10] + src0[3] * src1[14];
    dst[3] = src0[0] * src1[3] + src0[1] * src1[7] + src0[2] * src1[11] + src0[3] * src1[15];

    // row 1
    dst[4] = src0[4] * src1[0] + src0[5] * src1[4] + src0[6] * src1[8] + src0[7] * src1[12];
    dst[5] = src0[4] * src1[1] + src0[5] * src1[5] + src0[6] * src1[9] + src0[7] * src1[13];
    dst[6] = src0[4] * src1[2] + src0[5] * src1[6] + src0[6] * src1[10] + src0[7] * src1[14];
    dst[7] = src0[4] * src1[3] + src0[5] * src1[7] + src0[6] * src1[11] + src0[7] * src1[15];

    // row 2
    dst[8] = src0[8] * src1[0] + src0[9] * src1[4] + src0[10] * src1[8] + src0[11] * src1[12];
    dst[9] = src0[8] * src1[1] + src0[9] * src1[5] + src0[10] * src1[9] + src0[11] * src1[13];
    dst[10] = src0[8] * src1[2] + src0[9] * src1[6] + src0[10] * src1[10] + src0[11] * src1[14];
    dst[11] = src0[8] * src1[3] + src0[9] * src1[7] + src0[10] * src1[11] + src0[11] * src1[15];

    // row 3
    dst[12] = src0[12] * src1[0] + src0[13] * src1[4] + src0[14] * src1[8] + src0[15] * src1[12];
    dst[13] = src0[12] * src1[1] + src0[13] * src1[5] + src0[14] * src1[9] + src0[15] * src1[13];
    dst[14] = src0[12] * src1[2] + src0[13] * src1[6] + src0[14] * src1[10] + src0[15] * src1[14];
    dst[15] = src0[12] * src1[3] + src0[13] * src1[7] + src0[14] * src1[11] + src0[15] * src1[15];
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
        joints[j].s *= blendJoints[j].s * fraction;
    }
}

void BE_FASTCALL SIMD_Generic::ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) {
    int i;

    for (i = 0; i < numJoints; i++) {
        jointMats[i].SetScaleRotation(jointPoses[i].s, jointPoses[i].q.ToMat3());
        jointMats[i].SetTranslation(jointPoses[i].t);
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

void BE_FASTCALL SIMD_Generic::UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) {
    int i;

    for (i = lastJoint; i >= firstJoint; i--) {
        assert(parents[i] < i);
        jointMats[i].UntransformSelf(jointMats[parents[i]]);
    }
}

void BE_FASTCALL SIMD_Generic::MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) {
    int i;

    for (i = 0; i < numJoints; i++) {
        result[i] = joints1[i] * joints2[i];
    }
}

void BE_FASTCALL SIMD_Generic::TransformVerts(VertexGenericLit *verts, const int numVerts, const Mat3x4 *joints, const Vec4 *base, const int *index, const int numWeights) {
    const byte *jointsPtr = (byte *)joints;
    Vec3 v;
    int i, j;

    for (j = i = 0; i < numVerts; i++) {
        v = ((Mat3x4 *)(jointsPtr + index[j * 2 + 0]))->Transform(base[j]);
        while (index[j * 2 + 1] == 0) {
            j++;
            v += ((Mat3x4 *)(jointsPtr + index[j * 2 + 0]))->Transform(base[j]);
        }

        j++;

        verts[i].xyz = v;
    }
}

void BE_FASTCALL SIMD_Generic::DeriveTriPlanes(Plane *planes, const VertexGenericLit *verts, const int numVerts, const int *indexes, const int numIndexes) {
    for (int i = 0; i < numIndexes; i += 3) {
        const VertexGenericLit *a, *b, *c;
        float d0[3], d1[3], f;
        Vec3 n;

        a = verts + indexes[i + 0];
        b = verts + indexes[i + 1];
        c = verts + indexes[i + 2];

        d0[0] = b->xyz[0] - a->xyz[0];
        d0[1] = b->xyz[1] - a->xyz[1];
        d0[2] = b->xyz[2] - a->xyz[2];

        d1[0] = c->xyz[0] - a->xyz[0];
        d1[1] = c->xyz[1] - a->xyz[1];
        d1[2] = c->xyz[2] - a->xyz[2];

        n[0] = d1[1] * d0[2] - d1[2] * d0[1];
        n[1] = d1[2] * d0[0] - d1[0] * d0[2];
        n[2] = d1[0] * d0[1] - d1[1] * d0[0];

        f = Math::RSqrt(n.x * n.x + n.y * n.y + n.z * n.z);

        n.x *= f;
        n.y *= f;
        n.z *= f;

        planes->SetNormal(n);
        planes->FitThroughPoint(a->xyz);
        planes++;
    }
}

BE_NAMESPACE_END
