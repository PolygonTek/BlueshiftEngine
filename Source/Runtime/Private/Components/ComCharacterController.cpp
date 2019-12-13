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

#include "Precompiled.h"
#include "Render/Render.h"
#include "Physics/Collider.h"
#include "Components/ComTransform.h"
#include "Components/ComCollider.h"
#include "Components/ComRigidBody.h"
#include "Components/ComCharacterController.h"
#include "Game/GameWorld.h"
#include "Game/CastResult.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Character Controller", ComCharacterController, Component)
BEGIN_EVENTS(ComCharacterController)
END_EVENTS

void ComCharacterController::RegisterProperties() {
    REGISTER_PROPERTY("mass", "Mass", float, mass, 1.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 100, 0.1f);
    REGISTER_ACCESSOR_PROPERTY("capsuleRadius", "Capsule Radius", float, GetCapsuleRadius, SetCapsuleRadius, MeterToUnit(0.5f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("capsuleHeight", "Capsule Height", float, GetCapsuleHeight, SetCapsuleHeight, MeterToUnit(0.8f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("stepOffset", "Step Offset", float, GetStepOffset, SetStepOffset, CentiToUnit(40.0f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor).SetRange(0, CentiToUnit(50.0f), CentiToUnit(1.0f));
    REGISTER_ACCESSOR_PROPERTY("slopeLimit", "Slope Limit Angle", float, GetSlopeLimit, SetSlopeLimit, 60.0f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 90, 1);
}

ComCharacterController::ComCharacterController() {
    collider = nullptr;
    body = nullptr;
    correctionSensor = nullptr;
}

ComCharacterController::~ComCharacterController() {
    Purge(false);
}

bool ComCharacterController::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComCollider::metaObject)) {
        return true;
    }

    return false;
}

void ComCharacterController::Purge(bool chainPurge) {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
        collider = nullptr;
    }

    if (correctionSensor) {
        physicsSystem.DestroyCollidable(correctionSensor);
        correctionSensor = nullptr;
    }

    if (body) {
        physicsSystem.DestroyCollidable(body);
        body = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComCharacterController::Init() {
    Component::Init();

    ComTransform *transform = GetEntity()->GetTransform();
    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComCharacterController::TransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized.
    SetInitialized(true);
}

void ComCharacterController::Awake() {
    if (!body) {
        CreateBodyAndSensor();
    }
}

void ComCharacterController::CreateBodyAndSensor() {
    ComTransform *transform = GetEntity()->GetTransform();

    Vec3 center = Vec3(0, 0, capsuleRadius + capsuleHeight * 0.5f);
    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale() * capsuleRadius).MaxComponent();
    float scaledHeight = transform->GetScale().z * capsuleHeight;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCapsule(scaledCenter, scaledRadius, scaledHeight);

    PhysCollidableDesc desc;
    desc.type = PhysCollidable::Type::RigidBody;
    desc.character = true;
    desc.kinematic = true;
    desc.ccd = false;
    desc.mass = mass;
    desc.restitution = 0.5f;
    desc.friction = 1.0f;
    desc.rollingFriction = 1.0f;
    desc.spinningFriction = 1.0f;
    desc.linearDamping = 0.0f;
    desc.angularDamping = 0.0f;
    desc.origin = transform->GetOrigin();
    desc.axis = transform->GetAxis();

    PhysShapeDesc shapeDesc;
    shapeDesc.localOrigin.SetFromScalar(0);
    shapeDesc.localAxis.SetIdentity();
    shapeDesc.collider = collider;
    desc.shapes.Append(shapeDesc);

    body = (PhysRigidBody *)physicsSystem.CreateCollidable(desc);
    body->SetCollisionFilterBit(GetEntity()->GetLayer());
    body->SetAngularFactor(Vec3(0, 0, 0));
    body->SetCharacter(true);
    body->SetUserPointer(this);
    //body->SetCollisionListener(this);

    desc.type = PhysCollidable::Type::Sensor;
    desc.character = false;
    desc.kinematic = false;
    desc.ccd = false;
    desc.mass = 0.0f;
    desc.restitution = 0.0f;
    desc.friction = 0.0f;
    desc.rollingFriction = 0.0f;
    desc.spinningFriction = 0.0f;
    desc.linearDamping = 0.0f;
    desc.angularDamping = 0.0f;

    correctionSensor = (PhysSensor *)physicsSystem.CreateCollidable(desc);
    correctionSensor->SetDebugDraw(false);

    if (IsActiveInHierarchy()) {
        body->SetIgnoreCollisionCheck(*correctionSensor, true);

        body->AddToWorld(GetGameWorld()->GetPhysicsWorld());

        correctionSensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComCharacterController::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    origin = GetEntity()->GetTransform()->GetOrigin();

    RecoverFromPenetration();

    GroundTrace();

    GetEntity()->GetTransform()->SetOrigin(origin);
}

void ComCharacterController::GroundTrace() {
    Vec3 p1 = origin;
    Vec3 p2 = p1;

    // Move down 6 cm on the z axis to check if it is touching the ground.
    p2.z -= CentiToUnit(6.0f);

    int filterMask = GetGameWorld()->GetPhysicsWorld()->GetCollisionFilterMask(body->GetCollisionFilterBit());

    if (!GetGameWorld()->GetPhysicsWorld()->ConvexCast(body, collider, Mat3::identity, p1, p2, filterMask, groundTrace)) {
        onGround = false;
        isValidGroundTrace = false;
        return;
    }

    isValidGroundTrace = true;

    // FIXME: Should be compared with the surface normal, not the speration normal.
    if (groundTrace.normal.z < slopeDotZ) {
        onGround = false;
        return;
    }

    onGround = true;
}

void ComCharacterController::RecoverFromPenetration() {
    Array<Contact> contacts;

    int numPenetrationLoop = 0;
    while (1) {
        numPenetrationLoop++;
        // 최대 4 번까지 iteration
        if (numPenetrationLoop > 4) {
            break;
        }

        contacts.Clear();

        correctionSensor->SetOrigin(origin);
        correctionSensor->GetContacts(contacts);

        float maxPen = 0;

        for (int i = 0; i < contacts.Count(); i++) {
            Contact &contact = contacts[i];
            if (!contact.object->IsStatic()) {
                continue;
            }

            if (contact.dist < maxPen) {
                maxPen = contact.dist;

                // 한번에 밀어내지 않고, 가장 깊이 penetration 된 contact 부터 조금씩 밀어낸다.
                origin -= contact.normal * contact.dist * 0.25f;

                //BE_LOG("%s (%f) -> %s\n", contact.normal.ToString(), contact.dist, origin.ToString());
            }
        }

        // 다 밀어냈다면 종료
        if (maxPen == 0) {
            break;
        }
    }
}

void ComCharacterController::OnActive() {
    if (body) {
        body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
    if (correctionSensor) {
        correctionSensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComCharacterController::OnInactive() {
    if (body) {
        body->RemoveFromWorld();
    }
    if (correctionSensor) {
        correctionSensor->RemoveFromWorld();
    }
}

bool ComCharacterController::SlideMove(const Vec3 &moveVector) {
    const float backoffScale = 1.0001f;
    CastResultEx trace;
    Vec3 moveVec = moveVector;
    Vec3 slideVec;

    // Create a bump normal list.
    Vec3 bumpNormals[5];
    int numBumpNormals = 0;
    
    if (isValidGroundTrace) {
        // Slide along the ground floor.
        moveVec = moveVec.Slide(groundTrace.normal, backoffScale);

        // Add ground floor normal to the bump normal list.
        bumpNormals[numBumpNormals++] = groundTrace.normal;
    }

    if (moveVec.LengthSqr() == 0) {
        return false;
    }

    // Add starting move direction to the bump normal list.
    bumpNormals[numBumpNormals] = moveVec;
    bumpNormals[numBumpNormals].Normalize();
    numBumpNormals++;

    float f = 1.0f;
    for (int bumpCount = 0; bumpCount < 4; bumpCount++) {
        Vec3 targetPos = origin + moveVec * f;

        int filterMask = GetGameWorld()->GetPhysicsWorld()->GetCollisionFilterMask(body->GetCollisionFilterBit());
        // capsule cast from origin to targetPos.
        GetGameWorld()->GetPhysicsWorld()->ConvexCast(body, collider, Mat3::identity, origin, targetPos, filterMask, trace);

        // Move origin by a moveable fraction.
        if (trace.fraction > 0.0f) {
            origin = trace.endPos;
            numBumpNormals = 0;
        }
        
        // Exit if no collision occured after cast.
        if (trace.fraction == 1.0f) {
            break;
        }

        // Subtract the distance traveled by.
        f -= f * trace.fraction;

        //BE_LOG("%i %f\n", bumpCount, trace.normal.z);
        //GetGameWorld()->GetRenderWorld()->SetDebugColor(Vec4Color::cyan, Vec4(0, 0, 0, 0));
        //GetGameWorld()->GetRenderWorld()->DebugLine(trace.point, trace.point + trace.normal * 10, 1, false, 10000);

        // If the maximum number of accumulated bump normals is exceeded..
        if (numBumpNormals >= COUNT_OF(bumpNormals)) {
            // You should not come here. If you come, quit here.
            return true;
        }

        // If it hits the same plane, nudge moveVec slightly in the normal direction.
        int i = 0;
        for (; i < numBumpNormals; i++) {
            if (trace.normal.Dot(bumpNormals[i]) > 0.99f) {
                moveVec += trace.normal * 0.01f;
                break;
            }
        }

        if (i < numBumpNormals) {
            continue;
        }

        // Add bumpped normal.
        bumpNormals[numBumpNormals++] = trace.normal;

        for (int i = 0; i < numBumpNormals; i++) {
            // Except normal, which cannot be hit in the direction of moveVec.
            if (moveVec.Dot(bumpNormals[i]) >= 0.0f) {
                continue;
            }

            // Slide using bumpped normal.
            slideVec = moveVec.Slide(bumpNormals[i], backoffScale);

            for (int j = 0; j < numBumpNormals; j++) {
                if (j == i) {
                    continue;
                }

                if (slideVec.Dot(bumpNormals[j]) >= 0.0f) {
                    continue;
                }

                // Slide using another bumpped normal.
                slideVec = slideVec.Slide(bumpNormals[j], backoffScale);

                if (slideVec.Dot(bumpNormals[i]) >= 0.0f) {
                    continue;
                }
                
                // If the second slide vector collides with the first normal, it should be slided in the direction of the cross of the two normals.
                Vec3 slideDir = bumpNormals[i].Cross(bumpNormals[j]);
                slideVec = moveVec.ProjectTo(slideDir);

                slideVec += (bumpNormals[i] + bumpNormals[j]) * 0.01f;
                
                for (int k = 0; k < numBumpNormals; k++) {
                    if (k == i || k == j) {
                        continue;
                    }

                    if (slideVec.Dot(bumpNormals[k]) >= 0.0f) {
                        continue;
                    }

                    // Collided another normal again, stop the slide.
                    return true;
                }
            }

            moveVec = slideVec;
            break;
        }
    }

    return false;
}

bool ComCharacterController::Move(const Vec3 &moveVector) {
    if (!IsActiveInHierarchy()) {
        return false;
    }

    origin = GetEntity()->GetTransform()->GetOrigin();
    
    if (moveVector.z > 0.0f) {
        onGround = false;
        isValidGroundTrace = false;
    }

    bool stuck = SlideMove(moveVector);
    
    body->SetOrigin(origin);

    GetEntity()->GetTransform()->SetOrigin(origin);

    return stuck;
}

void ComCharacterController::TransformUpdated(const ComTransform *transform) {
    if (body) {
        body->SetOrigin(transform->GetOrigin());
        body->SetAxis(transform->GetAxis());
    }
}

float ComCharacterController::GetMass() const {
    return mass;
}

void ComCharacterController::SetMass(const float mass) {
    this->mass = mass;
}

float ComCharacterController::GetCapsuleRadius() const {
    return capsuleRadius;
}

void ComCharacterController::SetCapsuleRadius(const float capsuleRadius) {
    this->capsuleRadius = capsuleRadius;
}

float ComCharacterController::GetCapsuleHeight() const {
    return capsuleHeight;
}

void ComCharacterController::SetCapsuleHeight(const float capsuleHeight) {
    this->capsuleHeight = capsuleHeight;
}

float ComCharacterController::GetStepOffset() const {
    return stepOffset;
}

void ComCharacterController::SetStepOffset(const float stepOffset) {
    this->stepOffset = stepOffset;
}

float ComCharacterController::GetSlopeLimit() const {
    return RAD2DEG(Math::ACos(slopeDotZ));
}

void ComCharacterController::SetSlopeLimit(const float slopeLimit) {
    this->slopeDotZ = Math::Cos(DEG2RAD(slopeLimit));
}

#if WITH_EDITOR
void ComCharacterController::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(500.0f * 500.0f)) {
            float scaledRadius = (transform->GetScale() * capsuleRadius).MaxComponent();
            float scaledHeight = transform->GetScale().z * capsuleHeight;

            Vec3 localCenter = Vec3(0, 0, capsuleRadius + capsuleHeight * 0.5f);
            Vec3 worldCenter = transform->GetMatrix() * localCenter;

            renderWorld->SetDebugColor(Color4::yellow, Color4::zero);
            renderWorld->DebugCapsuleSimple(worldCenter, transform->GetAxis(), scaledHeight, scaledRadius, 1.0f, true);
        }
    }
}
#endif

BE_NAMESPACE_END
