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

    Anim State

    AnimState 는 하나의 애니메이션 트리를 이룬다.

    node 의 데이터는 AnimClip 이거나 AnimBlendTree 일 수 있다.

    nodeNum  < 0 ? AnimLeaf -> AnimClip      access from layer->GetAnimClip(nodeNum)
    nodeNum >= 0 ? AnimNode -> AnimBlendTree access from layer->GetAnimBlendTree(nodeNum)

---------------------------------------------------------------------------------
*/

#include "Containers/Array.h"
#include "Core/Str.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class AnimClip;
class AnimBlendTree;
class Animator;
class AnimLayer;
class JointPose;

struct AnimTimeEvent {
    float           time;
    Str             string;
};

class AnimState {
    friend class AnimLayer;
    friend class AnimStateBlender;

public:
    AnimState(AnimLayer *animLayer);
    AnimState(AnimLayer *animLayer, const AnimState *animState);

    AnimLayer *             GetAnimLayer() { return animLayer; }

                            /// Sets the name of anim state
    void                    SetName(const char *name) { this->name = name; }
                            /// Gets the name of anim state
    const Str &             GetName() const { return name; }

                            /// Checks if this anim state has single anim clip
    bool                    IsAnimClip() const;

                            /// Checks if this anim state has blend tree
    bool                    IsAnimBlendTree() const;

                            /// Returns anim clip
    AnimClip *              GetAnimClip() const;

                            /// Returns blend tree
    AnimBlendTree *         GetAnimBlendTree() const;

                            /// Sets this anim state to AnimClip
    int32_t                 SetAnimClip(AnimClip *animClip);

                            /// Sets this anim state to AnimBlendTree
    int32_t                 SetBlendTree(AnimBlendTree *animBlendTree);

                            /// Adds time event
    void                    AddTimeEvent(float time, const char *string);

                            /// Returns number of time events
    int                     NumTimeEvents() const { return events.Count(); }

                            /// Gets time event with the given index
    AnimTimeEvent &         GetTimeEvent(int index) { return events[index]; }

                            /// 2D X-Y position in the Editor
    const Vec2 &            GetNodePosition() const { return position; }
    void                    SetNodePosition(const Vec2 &position) { this->position = position; }

                            // 모든 서브 노드들을 blending 해서 duration 계산
    int                     GetDuration(const Animator *animator) const;

                            // 모든 서브 노드들을 blending 해서 masked joint pose 계산
    void                    GetFrame(const Animator *animator, float normalizedTime, int numJoints, JointPose *outJointPose) const;

                            // 모든 서브 노드들을 blending 해서 translation 계산
    void                    GetTranslation(const Animator *animator, float normalizedTime, Vec3 &outTranslation) const;

                            // 모든 서브 노드들을 blending 해서 rotation 계산
    void                    GetRotation(const Animator *animator, float normalizedTime, Quat &outRotation) const;

                            // 모든 서브 노드들의 AABB 의 합을 계산
    void                    GetAABB(const Animator *animator, float normalizedTime, AABB &outAabb) const;

private:
    Str                     name;           ///< state name
    int32_t                 nodeNum;
    Array<AnimTimeEvent>    events;
    Vec2                    position;       ///< position in the Editor

    AnimLayer *             animLayer;
};

BE_NAMESPACE_END
