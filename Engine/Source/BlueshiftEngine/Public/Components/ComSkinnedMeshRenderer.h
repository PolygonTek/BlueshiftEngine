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

#include "ComMeshRenderer.h"

BE_NAMESPACE_BEGIN

class SkeletonAsset;
class AnimAsset;
class Anim;

class ComSkinnedMeshRenderer : public ComMeshRenderer {
public:
    OBJECT_PROTOTYPE(ComSkinnedMeshRenderer);

    ComSkinnedMeshRenderer();
    virtual ~ComSkinnedMeshRenderer();

                            /// Returns true if this component conflicts with the given component
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    void                    UpdateAnimation(int time);

    Guid                    GetSkeletonGuid() const;
    void                    SetSkeletonGuid(const Guid &skeletonGuid);

    Guid                    GetAnimGuid() const;
    void                    SetAnimGuid(const Guid &animGuid);

    Anim *                  GetAnim() const { return anim; }

    int                     GetPlayStartTime() const { return playStartTime; }

protected:
    virtual void            UpdateVisuals() override;

    virtual void            MeshUpdated() override;

    void                    ChangeAnim(const Guid &animGuid);
    void                    AnimReloaded();

    void                    ChangeSkeleton(const Guid &skeletonGuid);
    void                    SkeletonReloaded();

    Skeleton *              skeleton;
    SkeletonAsset *         skeletonAsset;
    
    Anim *                  anim;
    AnimAsset *             animAsset;
    Guid                    animGuid;

    Array<int>              jointIndexes;
    Array<int>              jointParents;
    Mat3x4 *                jointMats;
    Array<AABB>             frameAABBs;

    int                     playStartTime;
};

BE_NAMESPACE_END
