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

    Animation Layer

    animation clip 이 tree 형태로 연결되어 있다.
    모든 node/leaf 데이터는 AnimLayer 에서 관리

    AnimState.nodeNum <  0 ? AnimLeaf -> AnimClip
    AnimState.nodeNum >= 0 ? AnimNode -> AnimBlendTree

    AnimBlendTree.nodeNum <  0 ? AnimLeaf -> AnimClip
    AnimBlendTree.nodeNum >= 0 ? AnimNode -> AnimBlendTree

---------------------------------------------------------------------------------
*/

#include "Containers/StaticArray.h"
#include "Core/Lexer.h"
#include "AnimState.h"
#include "AnimBlendTree.h"
#include "AnimClip.h"

BE_NAMESPACE_BEGIN

#define INVALID_ANIM_NODE           0x7FFFFFFF
#define IS_ANIM_NODE(nodeNum)       (((nodeNum) != INVALID_ANIM_NODE && (nodeNum) >= 0) ? true : false)
#define IS_ANIM_LEAF(nodeNum)       ((nodeNum) < 0 ? true : false)
#define TO_ANIM_LEAFNUM(nodeNum)    (-((nodeNum) + 1))

class AnimLayer {
    friend class AnimStateBlender;
    friend class Animator;
    friend class AnimController;

public:
    enum {
        MaxBlendTreeChildren    = 17
    };

    /// Layer blending types
    enum Blending {
        Override,
        Additive
    };

    struct AnimLeaf {
        Vec3                    blendSpaceVector;
        AnimClip *              animClip;
    };

    struct AnimNode {
        Vec3                    blendSpaceVector;
        AnimBlendTree *         animBlendTree;
        StaticArray<int32_t, MaxBlendTreeChildren> children;
    };

    struct Condition {
        enum CompareFunc {
            GreaterThan, GreaterEqual, LessThan, LessEqual, Equal
        };
        int                     parameterIndex;
        CompareFunc             compareFunc;
        float                   value;
    };

    struct AnimTransition {
        AnimState *             srcState;
        AnimState *             dstState;
        float                   exitTime;           ///< Normalized blend out exit time from src state
        float                   startTime;          ///< Normalized blend in start time from dst state
        float                   duration;           ///< Transition duration (Fixed seconds or normalized value from src state)
        bool                    fixedDuration;      ///< Transition duration is independent of state length
        bool                    hasExitTime;        ///< Transition has a fixed exit time
        bool                    isAtomic;           ///< Doesn't interrupt until transition finished
        Array<Condition>        conditions;         ///< Here we decide when transition get triggered
    };

    AnimLayer(AnimController *animController);
    AnimLayer(AnimController *animController, const AnimLayer *animLayer);
    ~AnimLayer();
                                
                                /// Returns total size of allocated memory
    size_t                      Allocated() const;
                                /// Returns total size of allocated memory including size of this type
    size_t                      Size() const;
                                /// Sets name of this layer
    void                        SetName(const char *name) { this->name = name; }
                                /// Returns name of this layer
    const Str &                 GetName() const { return name; }

    const Array<int> &          GetMaskJoints() const { return maskJoints; }

                                /// Gets blending type for blending with other layer
    Blending                    GetBlending() const { return blending; }
                                /// Sets blending type 
    void                        SetBlending(Blending blending) { this->blending = blending; }
                                /// Gets blendingweights for blending with other layer
    float                       GetWeight() const { return weight; }
                                /// Sets blending weight 
    void                        SetWeight(float weight) { this->weight = weight; }
    
                                /// Returns number of states
    int                         NumStates() const { return stateHashMap.Count(); }
                                /// Returns a state with the given index
    AnimState *                 GetState(int index) const { return stateHashMap.GetByIndex(index)->second; }
                                /// Returns a state with the given name
    AnimState *                 FindState(const char *name) const;
                                /// Creates a state with the given name
    AnimState *                 CreateState(const char *name);
                                /// Deletes a state
    void                        DeleteState(AnimState *state);
                                /// Returns a default state
    const AnimState *           GetDefaultState() const;
                                /// Sets a state to default
    void                        SetDefaultState(const AnimState *state);
    
    const AnimTransition *      FindTransition(const char *srcStateName, const char *dstStateName) const;
    bool                        ListTransitionsFrom(const char *srcStateName, Array<const AnimTransition *> &transitionArray) const;
    AnimTransition *            CreateTransition(const char *srcStateName, const char *dstStateName);
    bool                        RemoveTransition(AnimTransition *transition);

                                /// Returns a node with the given node number (greater than or equal to 0)
    AnimNode *                  GetNode(int32_t nodeNum);
                                /// Returns a leaf with the given node number (less than or equal to -1)
    AnimLeaf *                  GetLeaf(int32_t nodeNum);

    const Vec3                  GetNodeBlendSpaceVector(int32_t nodeNum) const;
    void                        SetNodeBlendSpaceVector(int32_t nodeNum, const Vec3 &blendSpaceVector);
    AnimBlendTree *             GetNodeAnimBlendTree(int32_t nodeNum) const;
    AnimClip *                  GetNodeAnimClip(int32_t nodeNum) const;

                                /// Creates a blend tree with the given name
    AnimBlendTree *             CreateBlendTree(const char *name);
                                /// Deletes a blend tree 
    void                        DeleteBlendTree(AnimBlendTree *animBlendTree);

                                /// Creates a node with given blend tree
    int32_t                     CreateNode(const Vec3 &blendSpaceVector, AnimBlendTree *animBlendTree);
                                /// Creates a leaf with given anim clip
    int32_t                     CreateLeaf(const Vec3 &blendSpaceVector, AnimClip *animClip);
                                /// Removes a node (or leaf)
    void                        RemoveNode(int32_t nodeNum);

    AnimController *            GetAnimController() const { return animController; }

private:
    bool                        ParseMaskJoints(Lexer &lexer);
    bool                        ParseState(Lexer &lexer);
    bool                        ParseBlendTree(Lexer &lexer, AnimBlendTree *blendTree);
    bool                        ParseEvents(Lexer &lexer, AnimState *state);
    bool                        ParseTransition(Lexer &lexer);
    void                        FreeData();

    Str                         name;
    Array<int>                  maskJoints;             // joints mask for this layer
    Blending                    blending;               // blending type for this layer
    float                       weight;                 // constant weight for this layer
    int                         defaultStateNum;

    StrIHashMap<AnimState *>    stateHashMap;

    Array<AnimBlendTree *>      blendTrees;

    Array<AnimNode *>           nodes;
    Array<AnimLeaf *>           leafs;

    Array<AnimTransition *>     transitions;
    
    AnimController *            animController;
};

BE_NAMESPACE_END
