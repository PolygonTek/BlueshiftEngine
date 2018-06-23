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

#include "Component.h"

BE_NAMESPACE_BEGIN

class Skeleton;
class Anim;
class SkeletonAsset;
class AnimAsset;

class ComAnimation : public Component {
public:
    OBJECT_PROTOTYPE(ComAnimation);

    ComAnimation();
    virtual ~ComAnimation();

                            /// Returns true if this component conflicts with the given component
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    void                    UpdateAnim(int time);

    Guid                    GetSkeletonGuid() const;
    void                    SetSkeletonGuid(const Guid &skeletonGuid);

    Guid                    GetAnimGuid(int index) const;
    void                    SetAnimGuid(int index, const Guid &animGuid);
    int                     GetAnimCount() const;
    void                    SetAnimCount(int count);

    float                   GetTimeOffset() const;
    void                    SetTimeOffset(float timeOffset);

    float                   GetTimeScale() const;
    void                    SetTimeScale(float timeScale);

    Skeleton *              GetSkeleton() const { return skeleton; }

    Mat3x4 *                GetJointMatrices() const { return jointMats; }

    const Anim *            GetCurrentAnim() const;

    float                   GetCurrentAnimSeconds() const;

protected:
    void                    ChangeAnim(int index, const Guid &animGuid);
    void                    AnimReloaded();

    void                    ChangeSkeleton(const Guid &skeletonGuid);
    void                    SkeletonReloaded();

    Skeleton *              skeleton;
    Array<Anim *>           anims;

    SkeletonAsset *         skeletonAsset;
    Array<AnimAsset *>      animAssets;

    Array<int>              jointIndexes;
    Array<int>              jointParents;
    Mat3x4 *                jointMats;

    int                     currentAnimIndex;
    int                     playStartTime;
    float                   timeOffset;
    float                   timeScale;
};

BE_NAMESPACE_END
