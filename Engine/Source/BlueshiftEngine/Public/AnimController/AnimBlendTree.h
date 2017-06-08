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

/*
---------------------------------------------------------------------------------

    Anim Blend Tree

    sub tree node

    node data ownership 은 AnimLayer 에 있음

---------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class AnimClip;
class AnimLayer;
class Animator;
class JointPose;
class File;

class AnimBlendTree {
public:
    enum BlendType {
        BlendAngle,
        Blend1D,
        Blend2DDirectional,
        Blend2DBarycentric, // TO BE IMPLEMENTED
        Blend3DBarycentric  // TO BE IMPLEMENTED
    };

    AnimBlendTree(AnimLayer *animLayer, int32_t nodeNum);
    AnimBlendTree(AnimLayer *animLayer, const AnimBlendTree *blendTree);

    const AnimLayer *       GetAnimLayer() const { return animLayer; }
    AnimLayer *             GetAnimLayer() { return animLayer; }

    const Str &             GetName() const { return name; }
    void                    SetName(const char *name) { this->name = name; }

    BlendType               GetBlendType() const { return blendType; }
    void                    SetBlendType(BlendType blendType) { this->blendType = blendType; }

    static int              BlendTypeDimensions(BlendType blendType);

                            // blend type 에 따라 사용되는 parameter 개수가 다르다
    int                     GetParameterIndex(int index) const { return parameterIndex[index]; }
    void                    SetParameterIndex(int index, int paramIndex) { parameterIndex[index] = paramIndex; }

    int32_t                 GetNodeNum() const { return nodeNum; }
    void                    SetNodeNum(int32_t nodeNum) { this->nodeNum = nodeNum; }

    int                     NumChildren() const;
                            // Get child node num
    int32_t                 GetChild(int childIndex) const;

    const Vec3              GetChildBlendSpaceVector(int childIndex) const;
    void                    SetChildBlendSpaceVector(int childIndex, const Vec3 &blendSpaceVector);

    void                    SetChildClip(int childIndex, AnimClip *animClip);
    void                    SetChildBlendTree(int childIndex, AnimBlendTree *animBlendTree);

    int32_t                 InsertChildClip(int childIndex, AnimClip *animClip, const Vec3 &blendSpaceVector);
    int32_t                 InsertChildBlendTree(int childIndex, AnimBlendTree *animBlendTree, const Vec3 &blendSpaceVector);
    int32_t                 AddChildClip(AnimClip *animClip, const Vec3 &blendSpaceVector);
    int32_t                 AddChildBlendTree(AnimBlendTree *animBlendTree, const Vec3 &blendSpaceVector);
    void                    RemoveChild(int childIndex);

                            // 모든 서브 노드들을 blending 해서 duration 계산
    float                   GetDuration(const Animator *animator) const;

                            // 모든 서브 노드들을 blending 해서 masked joint pose 계산
    void                    GetFrame(const Animator *animator, float normalizedTime, int numMaskJoints, const int *maskJoints, int numJoints, JointPose *outJointFrame) const;

                            // 모든 서브 노드들을 blending 해서 translation 계산
    void                    GetTranslation(const Animator *animator, float normalizedTime, Vec3 &outOrigin) const;

                            // 모든 서브 노드들을 blending 해서 rotation 계산
    void                    GetRotation(const Animator *animator, float normalizedTime, Quat &outRotation) const;

                            // 모든 서브 노드들의 AABB 의 합을 계산
    void                    GetAABB(const Animator *animator, float normalizedTime, AABB &aabb) const;

                            // 
    void                    Write(File *fp, const Str &indentSpace) const;

private:
    void                    ComputeChildrenWeights(const Animator *animator, float *weights) const;
    void                    ComputeChildrenBarycentricWeights(const Animator *animator, int blendType, float *weights) const;
    void                    ComputeChildren2DDirectionalWeights(const Animator *animator, float *weights) const;
    void                    ComputeChildrenAngleWeights(const Animator *animator, float *weights) const;

    Str                     name;
    int32_t                 nodeNum;
    BlendType               blendType;
    int                     parameterIndex[3];      // animator 에 등록된 parameter index, 최대 3개
    //DelaunayTriangles *     triangles;

    AnimLayer *             animLayer;
};

BE_NAMESPACE_END
