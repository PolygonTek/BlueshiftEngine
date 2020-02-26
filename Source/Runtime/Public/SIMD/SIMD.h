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

#include "Platform/cpuid.h"

/*
-------------------------------------------------------------------------------

    Single Instruction Multiple Data (SIMD)
    
-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class SIMD {
public:
    static void         Init(bool forceGeneric = false);
    static void         Shutdown();
};

/*
-------------------------------------------------------------------------------

    virtual base class for different SIMD processors

-------------------------------------------------------------------------------
*/

struct DominantTri;
struct VertexGeneric;
struct VertexGenericLit;

class Vec4;
class Plane;
class JointPose;
class CompressedJointPose;
class Mat3x4;

class BE_API SIMDProcessor {
public:
    SIMDProcessor() { cpuid = CPUID_NONE; }
    virtual ~SIMDProcessor() {}

    CpuId cpuid;

    virtual const char * BE_FASTCALL    GetName() const = 0;

    virtual void BE_FASTCALL            Add(float *dst, const float constant, const float *src, const int count) = 0;
    virtual void BE_FASTCALL            Add(float *dst, const float *src0, const float *src1, const int count) = 0;
    virtual void BE_FASTCALL            Sub(float *dst, const float constant, const float *src, const int count) = 0;
    virtual void BE_FASTCALL            Sub(float *dst, const float *src0, const float *src1, const int count) = 0;
    virtual void BE_FASTCALL            Mul(float *dst, const float constant, const float *src, const int count) = 0;
    virtual void BE_FASTCALL            Mul(float *dst, const float *src0, const float *src1, const int count) = 0;
    virtual void BE_FASTCALL            Div(float *dst, const float constant, const float *src, const int count) = 0;
    virtual void BE_FASTCALL            Div(float *dst, const float *src0, const float *src1, const int count) = 0;

    virtual float BE_FASTCALL           Sum(const float *src, const int count) = 0;

    virtual void BE_FASTCALL            TransposeMat4x4(float *dst, const float *src) = 0;
    virtual void BE_FASTCALL            MulMat3x4RM(float *dst, const float *src0, const float *src1) = 0;
    virtual void BE_FASTCALL            MulMat4x4RM(float *dst, const float *src0, const float *src1) = 0;
    virtual void BE_FASTCALL            MulMat4x4RMVec4(float *dst, const float *src0, const float *src1) = 0;

    virtual void BE_FASTCALL            Memcpy(void *dst, const void *src, const int count) = 0;
    virtual void BE_FASTCALL            Memset(void *dst, const int val, const int count) = 0;

    virtual void BE_FASTCALL            DecompressJoints(JointPose *joints, const CompressedJointPose *compressedJoints, const int *index, const int numJoints) = 0;
    virtual void BE_FASTCALL            AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) = 0;
    virtual void BE_FASTCALL            BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) = 0;
    virtual void BE_FASTCALL            BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) = 0;
    virtual void BE_FASTCALL            ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) = 0;
    virtual void BE_FASTCALL            ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints) = 0;
    virtual void BE_FASTCALL            TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) = 0;
    virtual void BE_FASTCALL            UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) = 0;
    virtual void BE_FASTCALL            MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) = 0;
    virtual void BE_FASTCALL            TransformVerts(VertexGenericLit *verts, const int numVerts, const Mat3x4 *joints, const Vec4 *weights, const int *index, const int numWeights) = 0;
};

extern SIMDProcessor *simdGeneric;
extern SIMDProcessor *simdProcessor;

BE_NAMESPACE_END

#include "SIMD_Generic.h"

#if defined(ENABLE_X86_SSE_INTRINSICS) || defined(ENABLE_ARM_NEON_INTRINSICS)
#include "SIMD_4.h"
#endif 

#if defined(ENABLE_X86_AVX_INTRINSICS)
#include "SIMD_8.h"
#endif
