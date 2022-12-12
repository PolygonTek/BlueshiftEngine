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

class btSoftBody;

BE_NAMESPACE_BEGIN

class PhysSoftBody : public PhysCollidable {
    friend class PhysicsSystem;

public:
    PhysSoftBody(btSoftBody *softBody, const Vec3 &centroid);
    virtual ~PhysSoftBody();

    virtual const Vec3      GetOrigin() const override;
    virtual void            SetOrigin(const Vec3 &origin) override;

    virtual const Mat3      GetAxis() const override;
    virtual void            SetAxis(const Mat3 &axis) override;

    virtual void            SetTransform(const Mat3x4 &transform) override;

    virtual void            AddToWorld(PhysicsWorld *physicsWorld) override;
    virtual void            RemoveFromWorld() override;

    float                   GetFriction() const;
    void                    SetFriction(float friction);

    float                   GetStiffness() const;
    void                    SetStiffness(float stiffness);

    float                   GetMass() const;
    void                    SetMass(float mass);

    const Vec3              GetGravity() const;
    void                    SetGravity(const Vec3 &gravityAcceleration);

    bool                    IsContinuousCollisionDetectionEnabled() const;
    void                    SetContinuousCollisionDetectionEnabled(bool enabled);

    bool                    IsSelfCollisionEnabled() const;
    void                    SetSelfCollisionEnabled(bool enabled);

    void                    AddForce(const Vec3 &force);
    void                    AddVelocity(const Vec3 &velocity);

    const Vec3              GetWindVelocity() const;
    void                    SetWindVelocity(const Vec3 &windVelocity);

    int                     GetPositionSolverIterationCount() const;
    void                    SetPositionSolverIterationCount(int iterationCount);

    int                     GetNodeCount() const;

    float                   GetNodeMass(int nodeIndex) const;
    void                    SetNodeMass(int nodeIndex, float mass);

    void                    AddNodeForce(int nodeIndex, const Vec3 &force);
    void                    AddNodeVelocity(int nodeIndex, const Vec3 &velocity);

    const Vec3              GetNodePosition(int nodeIndex) const;
    const Vec3              GetNodeNormal(int nodeIndex) const;

    void                    GetWorldAABB(AABB &worldAabb) const;

private:
    btSoftBody *            GetSoftBody();
    const btSoftBody *      GetSoftBody() const;
};

BE_NAMESPACE_END
