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

class Collider;
class ComRigidBody;
class ComSensor;

class ComCollider : public Component {
    friend class ComRigidBody;
    friend class ComSensor;

public:
    ABSTRACT_PROTOTYPE(ComCollider);

    ComCollider();
    virtual ~ComCollider() = 0;

                            /// Returns true if the same component is allowed
    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    Collider *              GetCollider() const { return collider; }

protected:
    virtual void            CreateCollider() = 0;

    Str                     material; // TODO: implement physics material
    Collider *              collider;
};

BE_NAMESPACE_END
