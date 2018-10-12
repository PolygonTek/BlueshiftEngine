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

class ComSkinnedMeshRenderer : public ComMeshRenderer {
    friend class LuaVM;

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

    Guid                    GetRootGuid() const;
    void                    SetRootGuid(const Guid &rootGuid);

    static const SignalDef  SIG_SkeletonUpdated;

protected:
    void                    UpdateSkeleton();

    virtual void            UpdateVisuals() override;

    virtual void            MeshUpdated() override;

    Guid                    rootGuid;
    Array<AABB>             frameAABBs;
};

BE_NAMESPACE_END
