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
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"
#include "Asset/GuidMapper.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "File/File.h"

BE_NAMESPACE_BEGIN

AnimBlendTree::AnimBlendTree(AnimLayer *animLayer, int32_t nodeNum) {
    this->animLayer         = animLayer;
    this->nodeNum           = nodeNum;
    this->blendType         = BlendType::Blend1D;
    this->parameterIndex[0] = -1;
    this->parameterIndex[1] = -1;
    this->parameterIndex[2] = -1;
}

AnimBlendTree::AnimBlendTree(AnimLayer *animLayer, const AnimBlendTree *animBlendTree) {
    this->animLayer         = animLayer;
    this->name              = animBlendTree->name;
    this->nodeNum           = animBlendTree->nodeNum;
    this->blendType         = animBlendTree->blendType;
    this->parameterIndex[0] = animBlendTree->parameterIndex[0];
    this->parameterIndex[1] = animBlendTree->parameterIndex[1];
    this->parameterIndex[2] = animBlendTree->parameterIndex[2];
}

void AnimBlendTree::SetChildBlendSpaceVector(int childIndex, const Vec3 &blendSpaceVector) {
    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    if (childIndex < 0 || childIndex >= node->children.Count()) {
        BE_WARNLOG(L"AnimBlendTree::SetChildBlendSpaceVector: childIndex out of range\n");
        return;
    }

    animLayer->SetNodeBlendSpaceVector(node->children[childIndex], blendSpaceVector);
}

const Vec3 AnimBlendTree::GetChildBlendSpaceVector(int childIndex) const {
    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    if (childIndex < 0 || childIndex >= node->children.Count()) {
        BE_WARNLOG(L"AnimBlendTree::GetChildBlendSpaceVector: childIndex out of range\n");
        return Vec3::zero;
    }

    return animLayer->GetNodeBlendSpaceVector(node->children[childIndex]);
}

int AnimBlendTree::NumChildren() const {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    return node->children.Count();
}

int32_t AnimBlendTree::GetChild(int childIndex) const {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    if (childIndex < 0 || childIndex >= node->children.Count()) {
        BE_WARNLOG(L"AnimBlendTree::GetChild: childIndex out of range\n");
        return INVALID_ANIM_NODE;
    }

    return node->children[childIndex];
}

void AnimBlendTree::SetChildClip(int childIndex, AnimClip *animClip) {
    int32_t childNodeNum = GetChild(childIndex);
    if (childNodeNum == INVALID_ANIM_NODE) {
        return;
    }

    if (IS_ANIM_NODE(childNodeNum)) {
        // TODO: remove
    }

    AnimLayer::AnimLeaf *childLeaf = animLayer->GetLeaf(childNodeNum);
    childLeaf->animClip = animClip;
}

void AnimBlendTree::SetChildBlendTree(int childIndex, AnimBlendTree *animBlendTree) {
    int32_t childNodeNum = GetChild(childIndex);
    if (childNodeNum == INVALID_ANIM_NODE) {
        return;
    }

    if (IS_ANIM_LEAF(childNodeNum)) {
        // TODO: remove
    }

    AnimLayer::AnimNode *childNode = animLayer->GetNode(childNodeNum);
    childNode->animBlendTree = animBlendTree;
}

int32_t AnimBlendTree::InsertChildClip(int index, AnimClip *animClip, const Vec3 &blendSpaceVector) {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    // Check the maximum children
    if (node->children.Count() >= AnimLayer::MaxBlendTreeChildren) {
        BE_WARNLOG(L"AnimBlendTree::AddChildClip: exceed MaxBlendTreeChildren\n");
        return INVALID_ANIM_NODE;
    }

    int32_t nodeNum = animLayer->CreateLeaf(blendSpaceVector, animClip);

    if (index < 0) {
        index = node->children.Count();
    }
    node->children.Insert(nodeNum, index);

    return nodeNum;
}

int32_t AnimBlendTree::AddChildClip(AnimClip *animClip, const Vec3 &blendSpaceVector) {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    int index = node->children.Count();
    return InsertChildClip(index, animClip, blendSpaceVector);
}

int32_t AnimBlendTree::InsertChildBlendTree(int index, AnimBlendTree *animBlendTree, const Vec3 &blendSpaceVector) {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    // Check the maximum children
    if (node->children.Count() >= AnimLayer::MaxBlendTreeChildren) {
        BE_WARNLOG(L"AnimBlendTree::AddChildBlendTree: exceed MaxBlendTreeChildren\n");
        return INVALID_ANIM_NODE;
    }

    int32_t nodeNum = animLayer->CreateNode(blendSpaceVector, animBlendTree);

    if (index < 0) {
        index = node->children.Count();
    }
    node->children.Insert(nodeNum, index);

    return nodeNum;
}

int32_t AnimBlendTree::AddChildBlendTree(AnimBlendTree *animBlendTree, const Vec3 &blendSpaceVector) {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    int index = node->children.Count();
    return InsertChildBlendTree(index, animBlendTree, blendSpaceVector);
}

void AnimBlendTree::RemoveChild(int childIndex) {
    AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);
    if (childIndex < 0 || childIndex >= node->children.Count()) {
        BE_WARNLOG(L"AnimBlendTree::RemoveChild: childIndex out of range\n");
        return;
    }

    animLayer->RemoveNode(node->children[childIndex]);

    node->children.RemoveIndex(childIndex);
}

int AnimBlendTree::BlendTypeDimensions(BlendType blendType) {
    switch (blendType) {
    case AnimBlendTree::BlendType::BlendAngle:
    case AnimBlendTree::BlendType::Blend1D:
        return 1;
    case AnimBlendTree::BlendType::Blend2DDirectional:
    case AnimBlendTree::BlendType::Blend2DBarycentric:
        return 2;
    case AnimBlendTree::BlendType::Blend3DBarycentric:
        return 3;
    }

    return 0;
}

// Sum of all the weights are equal to 1.0
void AnimBlendTree::ComputeChildrenWeights(const Animator *animator, float *weights) const {
    // TODO: Cache results if parameters are not changed
    if (blendType == AnimBlendTree::BlendType::Blend1D ||
        blendType == AnimBlendTree::BlendType::Blend2DBarycentric ||
        blendType == AnimBlendTree::BlendType::Blend3DBarycentric) {
        ComputeChildrenBarycentricWeights(animator, blendType, weights);
    } else if (blendType == AnimBlendTree::BlendType::Blend2DDirectional) {
        ComputeChildren2DDirectionalWeights(animator, weights);
    } else if (blendType == AnimBlendTree::BlendType::BlendAngle) {
        ComputeChildrenAngleWeights(animator, weights);
    }
}

void AnimBlendTree::ComputeChildrenBarycentricWeights(const Animator *animator, int blendType, float *weights) const {
    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    // Clears weights
    for (int i = 0; i < node->children.Count(); i++) {
        weights[i] = 0;
    }

    // Gets a current parametric point
    Vec3 currentPoint = Vec3::zero;
    for (int i = 0; i < 3; i++) {
        if (parameterIndex[i] >= 0) {
            currentPoint[i] = animator->GetParameterValue(parameterIndex[i]);
        }
    }

    // Gets blending sample points
    struct BlendNodePoint {
        int     childIndex;
        float   distanceSqr;
    };
    StaticArray<BlendNodePoint, AnimLayer::MaxBlendTreeChildren> samplePoints;
    for (int i = 0; i < node->children.Count(); i++) {
        BlendNodePoint blendNodePoint;
        blendNodePoint.childIndex = i;
        blendNodePoint.distanceSqr = currentPoint.DistanceSqr(animLayer->GetNodeBlendSpaceVector(node->children[i]));
        samplePoints.Append(blendNodePoint);
    }

    // Sort by nearest distance with currentPoint
    samplePoints.Sort([](const BlendNodePoint &p1, const BlendNodePoint &p2) -> bool {
        return p1.distanceSqr - p2.distanceSqr < 0;
    });

    if (blendType == AnimBlendTree::Blend2DBarycentric) {
        if (samplePoints.Count() >= 3) {
            int indexA = samplePoints[0].childIndex;
            int indexB = samplePoints[1].childIndex;
            int indexC = samplePoints[2].childIndex;
            
            Vec2 a = animLayer->GetNodeBlendSpaceVector(node->children[indexA]).ToVec2();
            Vec2 b = animLayer->GetNodeBlendSpaceVector(node->children[indexB]).ToVec2();
            Vec2 c = animLayer->GetNodeBlendSpaceVector(node->children[indexC]).ToVec2();
            Vec3 barycentricCoord = Vec3::Compute3DBarycentricCoords(a, b, c, currentPoint.ToVec2());

            weights[indexA] = barycentricCoord[0];
            weights[indexB] = barycentricCoord[1];
            weights[indexC] = barycentricCoord[2];
            //BE_LOG(L"(%f %f) %f (%f %f) %f (%f %f) %f\n", a.x, a.y, barycentricCoord.x, b.x, b.y, barycentricCoord.y, c.x, c.y, barycentricCoord.z);
            return;
        }
    }

    if (blendType == AnimBlendTree::Blend1D) {
        if (samplePoints.Count() >= 2) {
            int indexA = samplePoints[0].childIndex;
            int indexB = samplePoints[1].childIndex;

            float a = animLayer->GetNodeBlendSpaceVector(node->children[indexA])[0];
            float b = animLayer->GetNodeBlendSpaceVector(node->children[indexB])[0];
            Vec2 barycentricCoord = Vec2::Compute2DBarycentricCoords(a, b, currentPoint[0]);

            weights[indexA] = barycentricCoord[0];
            weights[indexB] = barycentricCoord[1];
            //BE_LOG(L"(%f) %f (%f) %f\n", a, barycentricCoord.x, b, barycentricCoord.y);
            return;
        }
    }

    if (samplePoints.Count() == 1) {
        weights[samplePoints[0].childIndex] = 1.0f;
    }
}

void AnimBlendTree::ComputeChildren2DDirectionalWeights(const Animator *animator, float *weights) const {
    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    // Clears weights
    for (int i = 0; i < node->children.Count(); i++) {
        weights[i] = 0;
    }

    // Gets a current parametric point
    Vec2 currentPoint = Vec2::zero;
    for (int i = 0; i < 2; i++) {
        if (parameterIndex[i] >= 0) {
            currentPoint[i] = animator->GetParameterValue(parameterIndex[i]);
        }
    }

    float currentAngle;
    float currentRadius = currentPoint.ToPolar(currentAngle);
    currentAngle = RAD2DEG(currentAngle);

    int originIndex = -1;

    // Gets blending sample points
    struct BlendNodePoint {
        int     childIndex;
        float   dot;
    };
    StaticArray<BlendNodePoint, AnimLayer::MaxBlendTreeChildren> samplePoints;
    for (int i = 0; i < node->children.Count(); i++) {
        Vec2 vec2 = animLayer->GetNodeBlendSpaceVector(node->children[i]).ToVec2();
        vec2.Normalize();
        if (vec2.IsZero()) {
            originIndex = i;
            continue;
        }

        BlendNodePoint blendNodePoint;
        blendNodePoint.childIndex = i;
        blendNodePoint.dot = currentPoint.Dot(vec2);
        samplePoints.Append(blendNodePoint);
    }

    // Sort by largest dot product with currentPoint
    samplePoints.Sort([](const BlendNodePoint &p1, const BlendNodePoint &p2) -> bool {
        return p1.dot - p2.dot > 0;
    });    

    if (samplePoints.Count() > 2) {
        int indexA = samplePoints[0].childIndex;
        int indexB = samplePoints[1].childIndex;

        if (originIndex != -1) {
            // If the sample point (0, 0) exist..
            Vec2 a = animLayer->GetNodeBlendSpaceVector(node->children[indexA]).ToVec2(); a.Normalize();
            Vec2 b = animLayer->GetNodeBlendSpaceVector(node->children[indexB]).ToVec2(); b.Normalize();
            Vec2 c = animLayer->GetNodeBlendSpaceVector(node->children[originIndex]).ToVec2();
            Vec3 barycentricCoord = Vec3::Compute3DBarycentricCoords(a, b, c, currentPoint);

            //BE_LOG(L"%f %f %f\n", barycentricCoord.x, barycentricCoord.y, barycentricCoord.z);

            bool isConvexHull = 
                barycentricCoord.x >= 0.0f && barycentricCoord.x <= 1.0f && 
                barycentricCoord.y >= 0.0f && barycentricCoord.y <= 1.0f &&
                barycentricCoord.z >= 0.0f && barycentricCoord.z <= 1.0f;

            // If current point is inside of a triangle, we'll use triangular interpolation
            if (!barycentricCoord.IsZero() && isConvexHull) {
                weights[indexA] = barycentricCoord[0];
                weights[indexB] = barycentricCoord[1];
                weights[originIndex] = barycentricCoord[2];
                return;
            }
        }

        float a = RAD2DEG(animLayer->GetNodeBlendSpaceVector(node->children[indexA]).ToVec2().ToAngle());
        float b = RAD2DEG(animLayer->GetNodeBlendSpaceVector(node->children[indexB]).ToVec2().ToAngle());
        float invDA = 1.0f / Math::Fabs(Math::AngleDelta(a, b));
        float wA = Math::Fabs(Math::AngleDelta(b, currentAngle)) * invDA;

        weights[indexA] = wA;
        weights[indexB] = 1.0f - wA;
        return;
    }

    if (samplePoints.Count() == 1) {
        weights[samplePoints[0].childIndex] = 1.0f;
    }
}

void AnimBlendTree::ComputeChildrenAngleWeights(const Animator *animator, float *weights) const {
    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    // Clears weights
    for (int i = 0; i < node->children.Count(); i++) {
        weights[i] = 0;
    }

    // Gets a current parametric angle
    float currentAngle = 0;
    if (parameterIndex[0] >= 0) {
        currentAngle = animator->GetParameterValue(parameterIndex[0]);
    }

    // Gets blending sample angles
    struct BlendNodePoint {
        int     childIndex;
        float   deltaAngle;
    };
    StaticArray<BlendNodePoint, AnimLayer::MaxBlendTreeChildren> samplePoints;
    for (int i = 0; i < node->children.Count(); i++) {
        BlendNodePoint blendNodePoint;
        blendNodePoint.childIndex = i;
        blendNodePoint.deltaAngle = Math::Fabs(Math::AngleDelta(currentAngle, animLayer->GetNodeBlendSpaceVector(node->children[i])[0]));
        samplePoints.Append(blendNodePoint);
    }

    // Sort by nearest angle with currentAngle
    samplePoints.Sort([](const BlendNodePoint &p1, const BlendNodePoint &p2) -> bool {
        return p1.deltaAngle - p2.deltaAngle < 0;
    });

    if (samplePoints.Count() >= 1) {
        if (samplePoints[0].deltaAngle == 0) {
            weights[samplePoints[0].childIndex] = 1.0f;
            return;
        }
    }

    if (samplePoints.Count() > 2) {
        int indexA = samplePoints[0].childIndex;
        int indexB = samplePoints[1].childIndex;

        float a = animLayer->GetNodeBlendSpaceVector(node->children[indexA])[0];
        float b = animLayer->GetNodeBlendSpaceVector(node->children[indexB])[0];
        float invDA = 1.0f / Math::Fabs(Math::AngleDelta(a, b));
        float wA = Math::Fabs(Math::AngleDelta(b, currentAngle)) * invDA;

        weights[indexA] = wA;
        weights[indexB] = 1.0f - wA;
        return;
    }

    if (samplePoints.Count() == 1) {
        weights[samplePoints[0].childIndex] = 1.0f;
    }
}

float AnimBlendTree::GetDuration(const Animator *animator) const {
    float weights[AnimLayer::MaxBlendTreeChildren] = { 0, };
    ComputeChildrenWeights(animator, weights);

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    float duration = 0;

    for (int i = 0; i < node->children.Count(); i++) {
        if (weights[i] > 0.0f) {
            int nodeNum = node->children[i];

            if (IS_ANIM_NODE(nodeNum)) {
                const AnimBlendTree *childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
                duration += weights[i] * childBlendTree->GetDuration(animator);
            } else {
                const AnimClip *childClip = animLayer->GetNodeAnimClip(nodeNum);
                duration += weights[i] * childClip->Length();
            }
        }
    }

    return duration;
}

void AnimBlendTree::GetFrame(const Animator *animator, float normalizedTime, int numMaskJoints, const int *maskJoints, int numJoints, JointPose *outJointFrame) const {
    Anim::FrameInterpolation    frameInterpolation;
    const AnimBlendTree *       childBlendTree;
    const AnimClip *            childClip;

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    if (node->children.Count() == 1) {
        int nodeNum = node->children[0];

        if (IS_ANIM_NODE(nodeNum)) {
            childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
            childBlendTree->GetFrame(animator, normalizedTime, numMaskJoints, maskJoints, numJoints, outJointFrame);
        } else {
            childClip = animLayer->GetNodeAnimClip(nodeNum);
            childClip->TimeToFrameInterpolation(normalizedTime * childClip->Length(), frameInterpolation);
            childClip->GetInterpolatedFrame(frameInterpolation, numMaskJoints, maskJoints, outJointFrame);
        }
    } else {
        float weights[AnimLayer::MaxBlendTreeChildren] = { 0, };
        ComputeChildrenWeights(animator, weights);

        JointPose *mixSrcFrame = (JointPose *)_alloca16(numJoints * sizeof(outJointFrame[0]));
        JointPose *ptr = outJointFrame;

        float blendedWeight = 0.0f;

        for (int i = 0; i < node->children.Count(); i++) {
            if (weights[i] > 0.0f) {
                int nodeNum = node->children[i];

                if (IS_ANIM_NODE(nodeNum)) {
                    childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
                    childBlendTree->GetFrame(animator, normalizedTime, numMaskJoints, maskJoints, numJoints, ptr);
                } else {
                    childClip = animLayer->GetNodeAnimClip(nodeNum);
                    childClip->TimeToFrameInterpolation(normalizedTime * childClip->Length(), frameInterpolation);
                    childClip->GetInterpolatedFrame(frameInterpolation, numMaskJoints, maskJoints, ptr);
                }

                blendedWeight += weights[i];
                
                // only blend after the first animation is mixed in
                if (ptr != outJointFrame) {
                    float fraction = weights[i] / blendedWeight;

                    simdProcessor->BlendJoints(outJointFrame, ptr, fraction, maskJoints, numMaskJoints);
                }

                ptr = mixSrcFrame;
            }
        }
    }
}

void AnimBlendTree::GetTranslation(const Animator *animator, float normalizedTime, Vec3 &outOrigin) const {
    const AnimBlendTree *   childBlendTree;
    const AnimClip *        childClip;

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    if (node->children.Count() == 1) {
        int nodeNum = node->children[0];

        if (IS_ANIM_NODE(nodeNum)) {
            childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
            childBlendTree->GetTranslation(animator, normalizedTime, outOrigin);
        } else {
            childClip = animLayer->GetNodeAnimClip(nodeNum);
            childClip->GetTranslation(normalizedTime * childClip->Length(), outOrigin);
        }
    } else {
        float weights[AnimLayer::MaxBlendTreeChildren] = { 0, };
        ComputeChildrenWeights(animator, weights);

        outOrigin.SetFromScalar(0);

        Vec3 offset;
        
        for (int i = 0; i < node->children.Count(); i++) {
            if (weights[i] > 0.0f) {
                int nodeNum = node->children[i];

                if (IS_ANIM_NODE(nodeNum)) {
                    childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
                    childBlendTree->GetTranslation(animator, normalizedTime, offset);
                } else {
                    childClip = animLayer->GetNodeAnimClip(nodeNum);
                    childClip->GetTranslation(normalizedTime * childClip->Length(), offset);
                }

                outOrigin += offset * weights[i];
            }
        }
    }
}

void AnimBlendTree::GetRotation(const Animator *animator, float normalizedTime, Quat &outRotation) const {
    const AnimBlendTree *   childBlendTree;
    const AnimClip *        childClip;
    float                   lerp;

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    if (node->children.Count() == 1) {
        int nodeNum = node->children[0];

        if (IS_ANIM_NODE(nodeNum)) {
            childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
            childBlendTree->GetRotation(animator, normalizedTime, outRotation);
        } else {
            childClip = animLayer->GetNodeAnimClip(nodeNum);
            childClip->GetRotation(normalizedTime * childClip->Length(), outRotation);
        }
    } else {
        float weights[AnimLayer::MaxBlendTreeChildren] = { 0, };
        ComputeChildrenWeights(animator, weights);

        outRotation.SetIdentity();

        Quat q;

        float blendedWeight = 0.0f;
        
        for (int i = 0; i < node->children.Count(); i++) {
            if (weights[i] > 0.0f) {
                blendedWeight += weights[i];
                lerp = weights[i] / blendedWeight;

                int nodeNum = node->children[i];

                if (IS_ANIM_NODE(nodeNum)) {
                    childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
                    childBlendTree->GetRotation(animator, normalizedTime, q);
                } else {
                    childClip = animLayer->GetNodeAnimClip(nodeNum);
                    childClip->GetRotation(normalizedTime * childClip->Length(), q);
                }

                outRotation.SetFromSlerp(outRotation, q, lerp);
            }
        }
    }
}

void AnimBlendTree::GetAABB(const Animator *animator, float normalizedTime, AABB &aabb) const {
    const AnimBlendTree *   childBlendTree;
    //const AnimClip *        childClip;

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    if (node->children.Count() == 1) {
        int nodeNum = node->children[0];

        if (IS_ANIM_NODE(nodeNum)) {
            childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
            childBlendTree->GetAABB(animator, normalizedTime, aabb);
        } else {
            //childClip = animLayer->GetNodeAnimClip(nodeNum);
            //int index = animator->GetAnimController()->FindAnimClipIndex(childClip);
            //childClip->GetAABB(normalizedTime * childClip->Length(), animator->GetAnimAABB(index)->frameAABBs, aabb);
            animator->GetMeshAABB(aabb);
        }
    } else {
        float weights[AnimLayer::MaxBlendTreeChildren] = { 0, };
        ComputeChildrenWeights(animator, weights);

        // children 의 time 은 child->duration * (time / nodeDuration) 에 맞춰서 흐른다

        aabb.Clear();

        AABB childAABB;
        
        for (int i = 0; i < node->children.Count(); i++) {
            if (weights[i] > 0.0f) {
                int nodeNum = node->children[i];

                if (IS_ANIM_NODE(nodeNum)) {
                    childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
                    childBlendTree->GetAABB(animator, normalizedTime, childAABB);
                } else {
                    //childClip = animLayer->GetNodeAnimClip(nodeNum);
                    //int index = animator->GetAnimController()->FindAnimClipIndex(childClip);
                    //childClip->GetAABB(normalizedTime * childClip->Length(), animator->GetAnimAABB(index)->frameAABBs, childAABB);
                    animator->GetMeshAABB(childAABB);
                }

                aabb += childAABB;
            }
        }
    }
}

void AnimBlendTree::Write(File *fp, const Str &indentSpace) const {
    int num = BlendTypeDimensions(blendType);

    fp->Printf("%sparameter", indentSpace.c_str());

    for (int i = 0; i < num; i++) {
        const AnimParm *animParm = animLayer->GetAnimController()->GetParameterByIndex(parameterIndex[i]);
        fp->Printf(" \"%s\"", animParm->name.c_str());
    }

    fp->Printf("\n");

    const AnimLayer::AnimNode *node = animLayer->GetNode(nodeNum);

    for (int i = 0; i < node->children.Count(); i++) {
        int nodeNum = node->children[i];

        const Vec3 blendSpaceVector = GetChildBlendSpaceVector(i);

        Str thresholdStr;
        switch (blendType) {
        case AnimBlendTree::BlendType::BlendAngle:
            thresholdStr = Str::FloatArrayToString((const float *)blendSpaceVector, 1, 3);
            break;
        case AnimBlendTree::BlendType::Blend1D:
            thresholdStr = Str::FloatArrayToString((const float *)blendSpaceVector, 1, 3);
            break;
        case AnimBlendTree::BlendType::Blend2DDirectional:
        case AnimBlendTree::BlendType::Blend2DBarycentric:
            thresholdStr = Str::FloatArrayToString((const float *)blendSpaceVector, 2, 3);
            break;
        case AnimBlendTree::BlendType::Blend3DBarycentric:
            thresholdStr = Str::FloatArrayToString((const float *)blendSpaceVector, 3, 3);
            break;
        }

        if (IS_ANIM_NODE(nodeNum)) {
            const AnimBlendTree *childBlendTree = animLayer->GetNodeAnimBlendTree(nodeNum);

            Str blendTypeStr;
            switch (childBlendTree->GetBlendType()) {
            case AnimBlendTree::BlendAngle:
                blendTypeStr = "blendAngle";
                break;
            case AnimBlendTree::Blend1D:
                blendTypeStr = "blend1D";
                break; 
            case AnimBlendTree::Blend2DDirectional:
                blendTypeStr = "blend2DDirectional";
                break;
            case AnimBlendTree::Blend2DBarycentric:
                blendTypeStr = "blend2DBarycentric";
                break;            
            case AnimBlendTree::Blend3DBarycentric:
                blendTypeStr = "blend3DBarycentric";
                break;
            }

            fp->Printf("%sblendTree ( %s ) \"%s\" %s {\n", indentSpace.c_str(), thresholdStr.c_str(), childBlendTree->GetName().c_str(), blendTypeStr.c_str());

            Str indentSpace2 = indentSpace;
            indentSpace2 += "  ";
            childBlendTree->Write(fp, indentSpace2);

            fp->Printf("%s}\n", indentSpace.c_str());
        } else if (IS_ANIM_LEAF(nodeNum)) {
            const AnimClip *childClip = animLayer->GetNodeAnimClip(nodeNum);
            const Guid animGuid = (!childClip || childClip->GetAnim()->IsDefaultAnim()) ? Guid() : resourceGuidMapper.Get(childClip->GetAnim()->GetHashName());

            fp->Printf("%sanimClip ( %s ) \"%s\"\n", indentSpace.c_str(), thresholdStr.c_str(), animGuid.ToString());
        } else {
            fp->Printf("%sanimClip ( %s ) \"%s\"\n", indentSpace.c_str(), thresholdStr.c_str(), Guid::zero.ToString());
        }
    }
}

BE_NAMESPACE_END
