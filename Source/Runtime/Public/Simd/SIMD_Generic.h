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

BE_NAMESPACE_BEGIN

class SIMD_Generic : public SIMDProcessor {
public:
    SIMD_Generic() { cpuid = CPUID_GENERIC; }

    virtual const char * BE_FASTCALL    GetName() const override { return "SIMD Generic"; }

    virtual void BE_FASTCALL            Add(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Add(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Sub(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Sub(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Mul(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Mul(float *dst, const float *src0, const float *src1, const int count) override;
    virtual void BE_FASTCALL            Div(float *dst, const float constant, const float *src, const int count) override;
    virtual void BE_FASTCALL            Div(float *dst, const float *src0, const float *src1, const int count) override;

    virtual float BE_FASTCALL           Sum(const float *src, const int count) override;

    virtual void BE_FASTCALL            TransposeMat4x4(float *dst, const float *src) override;
    virtual void BE_FASTCALL            MulMat3x4RM(float *dst, const float *src0, const float *src1) override;
    virtual void BE_FASTCALL            MulMat4x4RM(float *dst, const float *src0, const float *src1) override;
    virtual void BE_FASTCALL            MulMat4x4RMVec4(float *dst, const float *src0, const float *src1) override;

    virtual void BE_FASTCALL            Memcpy(void *dst, const void *src, const int count) override;
    virtual void BE_FASTCALL            Memset(void *dst, const int val, const int count) override;

    virtual void BE_FASTCALL            DecompressJoints(JointPose *joints, const CompressedJointPose *compressedJoints, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            AdditiveBlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            BlendJoints(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            BlendJointsFast(JointPose *joints, const JointPose *blendJoints, const float fraction, const int *index, const int numJoints) override;
    virtual void BE_FASTCALL            ConvertJointPosesToJointMats(Mat3x4 *jointMats, const JointPose *jointPoses, const int numJoints) override;
    virtual void BE_FASTCALL            ConvertJointMatsToJointPoses(JointPose *jointPoses, const Mat3x4 *jointMats, const int numJoints) override;
    virtual void BE_FASTCALL            TransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) override;
    virtual void BE_FASTCALL            UntransformJoints(Mat3x4 *jointMats, const int *parents, const int firstJoint, const int lastJoint) override;
    virtual void BE_FASTCALL            MultiplyJoints(Mat3x4 *result, const Mat3x4 *joints1, const Mat3x4 *joints2, const int numJoints) override;
    virtual void BE_FASTCALL            TransformVerts(VertexGenericLit *verts, const int numVerts, const Mat3x4 *joints, const Vec4 *weights, const int *index, const int numWeights) override;
};

BE_NAMESPACE_END
