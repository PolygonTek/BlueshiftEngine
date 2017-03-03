#pragma once

#include "SSE/sse.h"

BE_NAMESPACE_BEGIN

typedef sseb simd4b;
typedef ssef simd4f;
typedef ssei simd4i;

class SIMD_SSE4 : public SIMD_Generic {
public:
    SIMD_SSE4() { cpuid = CPUID_SSE4; }

    virtual const char * BE_FASTCALL    GetName() const { return "SSE4"; }

    virtual void BE_FASTCALL            Add(float *dst, const float constant, const float *src, const int count);
    virtual void BE_FASTCALL            Add(float *dst, const float *src0, const float *src1, const int count);
    virtual void BE_FASTCALL            Sub(float *dst, const float constant, const float *src, const int count);
    virtual void BE_FASTCALL            Sub(float *dst, const float *src0, const float *src1, const int count);
    virtual void BE_FASTCALL            Mul(float *dst, const float constant, const float *src, const int count);
    virtual void BE_FASTCALL            Mul(float *dst, const float *src0, const float *src1, const int count);
    virtual void BE_FASTCALL            Div(float *dst, const float constant, const float *src, const int count);
    virtual void BE_FASTCALL            Div(float *dst, const float *src0, const float *src1, const int count);

    virtual float BE_FASTCALL           Sum(const float *src, const int count);

    virtual void BE_FASTCALL            MatrixTranspose(float *dst, const float *src);
    virtual void BE_FASTCALL            MatrixMultiply(float *dst, const float *src0, const float *src1);

    /*virtual void BE_FASTCALL            BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints);
    virtual void BE_FASTCALL            BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints);
    virtual void BE_FASTCALL            ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints);
    virtual void BE_FASTCALL            ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints);
    virtual void BE_FASTCALL            TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint);
    virtual void BE_FASTCALL            UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint);*/
};

BE_NAMESPACE_END
