#pragma once

BE_NAMESPACE_BEGIN

class SIMD_Generic : public SIMDProcessor {
public:
    SIMD_Generic() { cpuid = CPUID_GENERIC; }

    virtual const char * BE_FASTCALL    GetName() const { return "generic code"; }

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

    virtual void BE_FASTCALL            Memcpy(void *dst, const void *src, const int count);
    virtual void BE_FASTCALL            Memset(void *dst, const int val, const int count);

    virtual void BE_FASTCALL            DecompressJoints(JointPose *joints, const CompressedJointPose *compressedJoints, const int *index, const int numJoints);
    virtual void BE_FASTCALL            AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints);
    virtual void BE_FASTCALL            BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints);
    virtual void BE_FASTCALL            BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints);
    virtual void BE_FASTCALL            ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints);
    virtual void BE_FASTCALL            ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints);
    virtual void BE_FASTCALL            TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint);
    virtual void BE_FASTCALL            UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint);
    virtual void BE_FASTCALL            MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints);
    virtual void BE_FASTCALL            TransformVerts(VertexLightingGeneric *verts, const int numVerts, const Mat3x4 *joints, const Vec4 *weights, const int *index, const int numWeights);
    virtual void BE_FASTCALL            DeriveTriPlanes(Plane *planes, const VertexLightingGeneric *verts, const int numVerts, const int *indexes, const int numIndexes);
};

BE_NAMESPACE_END
