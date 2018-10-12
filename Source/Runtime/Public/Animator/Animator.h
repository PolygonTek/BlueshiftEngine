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

    Animator

    animation context for animating AnimController

    animation blending has different use cases:

    1. Parametric blend tree: 
       Each animation state involves animation blend tree.
       tree 구조로 구축된 animation 들을 parameter 조절로 blending.
       (ex. directional blending, blending between walk and run)
       To make synchronize blending two animations should have almost same 
       normalized times. (TODO: Sometimes two animations can not have same
       normalized times, so wee need to define sync points)
    
    2. Transitional blending: 
       animation state 간의 blending. source animation fade out and destination 
       animation fade in. (ex. 갑작스레 행동이 바뀌었을 때 일어나는 blending)

    3. Partial blending: 
       base layer 외의 추가적인 layer 에 의한 blending. 부위별 masked blending or 
       여러 동작들의 중첩 blending (additive)

---------------------------------------------------------------------------------
*/

#include "Math/Math.h"
#include "Containers/Array.h"
#include "AnimStateBlender.h"

BE_NAMESPACE_BEGIN

class Str;
class AnimController;
class AnimLayer;
class Mesh;
class Mat3x4;
class Entity;

class Animator {
public:
    enum {
        MaxBlendersPerLayer = 4,
        MaxLayers           = 32
    };

    struct AnimAABB {
        Array<AABB>         frameAABBs;
    };

    Animator();
    ~Animator();

                            /// Returns total size of allocated memory
    size_t                  Allocated() const;
                            /// Returns total size of allocated memory including size of this type
    size_t                  Size() const;

    int                     GetParameterIndex(const char *parmName) const;

    const float             GetParameterValue(int index) const;

    void                    SetParameterValue(int index, const float value);
    bool                    SetParameterValue(const char *parmName, const float value);

                            // auto transitions happens in here
    void                    UpdateFrame(Entity *entity, int previousTime, int currentTime);

    void                    ClearAnimController();
    void                    SetAnimController(const Str &name);
    AnimController *        GetAnimController() const { return animController; }

                            /// Returns the number of joints
    int                     NumJoints() const { return numJoints; }
                            /// Returns the joint name with the given joint index
    const char *            GetJointName(int jointIndex) const;
                            /// Returns the joint index with the given joint name
    int                     GetJointIndex(const char *jointName) const;

                            /// Returns the number of layers
    int                     NumAnimLayers() const;
                            /// Returns layer pointer with the given layer index
    const AnimLayer *       GetAnimLayer(int layerIndex) const;

    const AnimAABB *        GetAnimAABB(int index) const { return &animAABBs[index]; }

                            // mesh 로 skinning 된 anim controller 의 모든 anim clip 에 대해 AABB 를 계산한다. 
    void                    ComputeAnimAABBs(const Mesh *mesh);

    void                    ResetState(int currentTime);

    const AnimState *       CurrentAnimState(int layerNum) const;
    
    void                    TransitState(int layerNum, const char *stateName, int currentTime, float startOffset, int blendDuration, bool isAtomic);

                            // 모든 blending 을 계산한 current time 의 joint matrices 를 만든다 
    void                    ComputeFrame(int currentTime);

                            // ComputeFrame() 결과 행렬들을 리턴
    Mat3x4 *                GetFrame() const { return jointMats; }

                            // 모든 blending 을 계산한 current time 의 root bone 의 translation 을 구한다
    void                    GetTranslation(int currentTime, Vec3 &translation) const;

                            // 모든 blending 을 계산한 current time 의 root bone 의 translation delta 를 구한다
    void                    GetTranslationDelta(int fromTime, int toTime, Vec3 &translationDelta) const;

                            // 모든 blending 을 계산한 current time 의 root bone 의 rotation delta 를 구한다
    bool                    GetRotationDelta(int fromTime, int toTime, Mat3 &rotationDelta) const;
        
                            // 모든 blending 을 계산한 current time 의 AABB 를 구한다
    void                    ComputeAABB(int currentTime);

                            // CoumputeAABB() 결과를 리턴
    void                    GetAABB(AABB &aabb) const;

    void                    GetMeshAABB(AABB &aabb) const { aabb = meshAABB; }

private:
    void                    PushStateBlenders(int layerNum, int currentTime, int blendDuration);
    void                    FreeData();

    AnimController *        animController;
    Array<AnimAABB>         animAABBs;
    AABB                    meshAABB;               // TEMP: to be replaced by animAABBs

    int                     numJoints;              // number of joints
    Mat3x4 *                jointMats;              // result of ComputeFrame() 
    AABB                    frameAABB;
    
    bool                    ignoreRootTranslation;

    Array<float>            parameters;
    AnimStateBlender        layerAnimStateBlenders[MaxLayers][MaxBlendersPerLayer];
};

BE_NAMESPACE_END
