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

#include "Animator/Animator.h"
#include "ComMeshRenderer.h"

BE_NAMESPACE_BEGIN

class AnimControllerAsset;

class ComAnimator : public ComMeshRenderer {
public:
    OBJECT_PROTOTYPE(ComAnimator);

    ComAnimator();
    virtual ~ComAnimator();

                            /// Returns true if this component conflicts with the given component
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    void                    UpdateAnimation(int time);

    Vec3                    GetTranslation(int currentTime) const;

    Vec3                    GetTranslationDelta(int fromTime, int toTime) const;

    Mat3                    GetRotationDelta(int fromTime, int toTime) const;

    void                    SetAnimParameter(const char *parm, float value);

    const char *            GetCurrentAnimState(int layerNum) const;

    void                    ResetAnimState();

    void                    TransitAnimState(int layerNum, const char *stateName, int blendOffset, int blendDuration, bool isAtomic);

    Animator &              GetAnimator() { return animator; }

    Guid                    GetAnimControllerGuid() const;
    void                    SetAnimControllerGuid(const Guid &animControllerGuid);

protected:
    virtual void            UpdateVisuals() override;

    virtual void            MeshUpdated() override;

    void                    ChangeAnimController(const Guid &animControllerGuid);
    void                    AnimControllerReloaded();

    Animator                animator;
    AnimControllerAsset *   animControllerAsset;
};

BE_INLINE Vec3 ComAnimator::GetTranslation(int currentTime) const {
    Vec3 translation = Vec3::zero;
    animator.GetTranslation(currentTime, translation);
    return translation;
}

BE_INLINE Vec3 ComAnimator::GetTranslationDelta(int fromTime, int toTime) const {
    Vec3 delta = Vec3::zero;
    animator.GetTranslationDelta(fromTime, toTime, delta);
    return delta;
}

BE_INLINE Mat3 ComAnimator::GetRotationDelta(int fromTime, int toTime) const {
    Mat3 delta = Mat3::identity;
    animator.GetRotationDelta(fromTime, toTime, delta);
    return delta;
}

BE_INLINE void ComAnimator::SetAnimParameter(const char *parm, float value) {
    animator.SetParameterValue(parm, value);
}

BE_NAMESPACE_END
