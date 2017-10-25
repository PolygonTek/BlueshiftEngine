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

    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Update() override;

    void                    UpdateAnimation(int time);

    Anim *                  GetAnim() const { return anim; }

    Guid                    GetAnimGuid() const;
    void                    SetAnimGuid(const Guid &animGuid);

    Guid                    GetSkeletonGuid() const;
    void                    SetSkeletonGuid(const Guid &skeletonGuid);

    int                     GetPlayStartTime() const { return playStartTime; }

protected:
    virtual void            UpdateVisuals() override;

    virtual void            MeshUpdated() override;

    void                    ChangeAnim(const Guid &animGuid);
    void                    AnimReloaded();

    void                    ChangeSkeleton(const Guid &skeletonGuid);
    void                    SkeletonReloaded();

    void                    PropertyChanged(const char *classname, const char *propName);
    
    Skeleton *              skeleton;
    SkeletonAsset *         skeletonAsset;
    
    Anim *                  anim;
    AnimAsset *             animAsset;

    BE1::Array<int>         jointIndexes;
    BE1::Array<int>         jointParents;
    Mat3x4 *                jointMats;
    BE1::Array<AABB>        frameAABBs;

    int                     playStartTime;
};

BE_NAMESPACE_END
