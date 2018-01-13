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
    REGISTER_PROPERTY("mass", "Mass", float, mass, 1.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 100, 0.1f);
    REGISTER_ACCESSOR_PROPERTY("capsuleRadius", "Capsule Radius", float, GetCapsuleRadius, SetCapsuleRadius, 0.5f, "", PropertyInfo::EditorFlag)
        .SetRange(0.01, 2, 0.01);
    REGISTER_ACCESSOR_PROPERTY("capsuleHeight", "Capsule Height", float, GetCapsuleHeight, SetCapsuleHeight, 0.8f, "", PropertyInfo::EditorFlag)
        .SetRange(0.01, 2, 0.01);
    REGISTER_ACCESSOR_PROPERTY("stepOffset", "Step Offset", float, GetStepOffset, SetStepOffset, 0.5f, "", PropertyInfo::EditorFlag)
        .SetRange(0.0, 1.0, 0.1);
    REGISTER_ACCESSOR_PROPERTY("slopeLimit", "Slope Limit Angle", float, GetSlopeLimit, SetSlopeLimit, 60.0f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 90, 1);
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

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComCharacterController::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);
}

void ComCharacterController::Awake() {
    if (!body) {
        ComTransform *transform = GetEntity()->GetTransform();

        Vec3 center = Vec3(0, 0, capsuleRadius + capsuleHeight * 0.5f);
        Vec3 scaledCenter = transform->GetScale() * center;
        float scaledRadius = (transform->GetScale() * capsuleRadius).MaxComponent();
        float scaledHeight = transform->GetScale().z * capsuleHeight;

        collider = colliderManager.AllocUnnamedCollider();
        collider->CreateCapsule(scaledCenter, scaledRadius, scaledHeight, scaledRadius);

        PhysCollidableDesc desc;
        desc.type = PhysCollidable::Type::Character;
        desc.kinematic = true;
        desc.ccd = false;
        desc.mass = mass;
        desc.restitution = 0.5f;
        desc.friction = 1.0f;
        desc.rollingFriction = 1.0f;
        desc.linearDamping = 0.0f;
        desc.angularDamping = 0.0f;
        desc.origin = transform->GetOrigin();
        desc.axis = transform->GetAxis();

        PhysShapeDesc shapeDesc;
        shapeDesc.localOrigin.SetFromScalar(0);
        shapeDesc.localAxis.SetIdentity();
        shapeDesc.collider = collider;
        desc.shapes.Append(shapeDesc);

        body = (PhysRigidBody *)physicsSystem.CreateCollidable(&desc);
        body->SetCustomCollisionFilterIndex(GetEntity()->GetLayer());
        body->SetAngularFactor(Vec3(0, 0, 0));
        body->SetCharacter(true);
        body->SetUserPointer(this);
        //body->SetCollisionListener(this);
        
        desc.type = PhysCollidable::Sensor;
        desc.kinematic = false;
        desc.ccd = false;
        desc.mass = 0.0f;
        desc.restitution = 0.0f;
        desc.friction = 0.0f;
        desc.rollingFriction = 0.0f;
        desc.linearDamping = 0.0f;
        desc.angularDamping = 0.0f;

        correctionSensor = (PhysSensor *)physicsSystem.CreateCollidable(&desc);
        correctionSensor->SetDebugDraw(false);

        if (IsActiveInHierarchy()) {
            body->SetIgnoreCollisionCheck(*correctionSensor, true);

            body->AddToWorld(GetGameWorld()->GetPhysicsWorld());

            correctionSensor->AddToWorld(GetGameWorld()->GetPhysicsWorld());
        }
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

void ComCharacterController::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
            Vec3 center = Vec3(0, 0, capsuleRadius + capsuleHeight * 0.5f);
            float scaledRadius = (transform->GetScale() * capsuleRadius).MaxComponent();
            float scaledHeight = transform->GetScale().z * capsuleHeight;

            Vec3 worldCenter = transform->GetTransform() * center;

            renderWorld->SetDebugColor(Color4::yellow, Color4::zero);
            renderWorld->DebugCapsuleSimple(worldCenter, transform->GetAxis(), scaledHeight, scaledRadius, 1.0f, true);
        }
    }
}

void ComCharacterController::GroundTrace() {
    Vec3 p1 = origin;
    Vec3 p2 = p1;

    // 땅에 닿아있는지 체크하기위해 z 축으로 1.5cm 만큼 내려서 이동시켜 본다
    p2.z -= CentiToUnit(1.5);
    if (!GetGameWorld()->GetPhysicsWorld()->ConvexCast(body, collider, Mat3::identity, p1, p2, 
        PhysCollidable::CharacterGroup, PhysCollidable::StaticGroup | PhysCollidable::DefaultGroup, groundTrace)) {
        onGround = false;
        isValidGroundTrace = false;
        return;
    }

    isValidGroundTrace = true;

    // FIXME: speration normal 이 아닌 표면의 normal 과 비교해야 한다
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
            }
        }

        // 다 밀어냈다면 종료
        if (maxPen == 0) {
            break;
        }
    }
}

bool ComCharacterController::SlideMove(const Vec3 &moveVector) {
    const float backoffScale = 1.001f;
    CastResultEx trace;
    Vec3 moveVec = moveVector;
    Vec3 slideVec;

    // bump normal 리스트를 작성한다. 
    Vec3 bumpNormals[5];
    int numBumpNormals = 0;
    
    if (isValidGroundTrace) {
        // 땅 바닥과 슬라이드
        moveVec = moveVec.Slide(groundTrace.normal, backoffScale);

        // 땅 바닥 normal 을 bump normal 리스트에 추가
        bumpNormals[numBumpNormals++] = groundTrace.normal;
    }

    if (moveVec.LengthSqr() == 0) {
        return false;
    }

    // 시작 move 방향을 bump normal 리스트에 추가
    bumpNormals[numBumpNormals] = moveVec;
    bumpNormals[numBumpNormals].Normalize();
    numBumpNormals++;

    float f = 1.0f;
    for (int bumpCount = 0; bumpCount < 4; bumpCount++) {
        Vec3 targetPos = origin + moveVec * f;

        // origin 에서 targetPos 로 capsule cast 
        GetGameWorld()->GetPhysicsWorld()->ConvexCast(body, collider, Mat3::identity, origin, targetPos, 
            PhysCollidable::CharacterGroup, PhysCollidable::DefaultGroup | PhysCollidable::StaticGroup, trace);

        // 이동 가능한 fraction 만큼 origin 이동
        if (trace.fraction > 0.0f) {
            origin = trace.endpos;
            numBumpNormals = 0;
        }
        
        // cast 결과 충돌이 없다면 종료
        if (trace.fraction == 1.0f) {
            break;
        }

        // 이동한 만큼 이동거리를 빼준다.
        f -= f * trace.fraction;

        //BE_LOG(L"%i %f\n", bumpCount, trace.normal.z);
        //GetGameWorld()->GetRenderWorld()->SetDebugColor(Vec4Color::cyan, Vec4(0, 0, 0, 0));
        //GetGameWorld()->GetRenderWorld()->DebugLine(trace.point, trace.point + trace.normal * 10, 1, false, 10000);

        // 누적된 bump normals 의 최대 개수를 초과했다면..
        if (numBumpNormals >= COUNT_OF(bumpNormals)) {
            // 여기로 오면 안된다. 혹시나 만약 오게되면 여기서 종료.
            return true;
        }

        // 같은 평면에 부딛혔다면 moveVec 을 normal 방향으로 약간 nudge 시킨다.
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

        // 부딪힌 normal 을 추가
        bumpNormals[numBumpNormals++] = trace.normal;

        for (int i = 0; i < numBumpNormals; i++) {
            // moveVec 방향으로 부딪힐 수 없는 normal 은 제외
            if (moveVec.Dot(bumpNormals[i]) >= 0.0f) {
                continue;
            }

            // 부딪힌 normal 로 slide
            slideVec = moveVec.Slide(bumpNormals[i], backoffScale);

            for (int j = 0; j < numBumpNormals; j++) {
                if (j == i) {
                    continue;
                }

                if (slideVec.Dot(bumpNormals[j]) >= 0.0f) {
                    continue;
                }

                // 또 다시 다른 평면으로 slide 
                slideVec = slideVec.Slide(bumpNormals[j], backoffScale);

                if (slideVec.Dot(bumpNormals[i]) >= 0.0f) {
                    continue;
                }
                
                // 두번째 slide vector 가 첫번째 부딛힌 normal 과 또 부딛힌다면, cross 방향으로 slide
                Vec3 slideDir = bumpNormals[i].Cross(bumpNormals[j]);
                slideDir.Normalize();
                slideVec = slideDir * slideDir.Dot(moveVec);

                slideVec += (bumpNormals[i] + bumpNormals[j]) * 0.01f;
                
                for (int k = 0; k < numBumpNormals; k++) {
                    if (k == i || k == j) {
                        continue;
                    }

                    if (slideVec.Dot(bumpNormals[k]) >= 0.0f) {
                        continue;
                    }

                    // 다른 normal 과 또 부딛힌다면 slide 를 멈춘다.
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
    return UnitToMeter(capsuleRadius);
}

void ComCharacterController::SetCapsuleRadius(const float capsuleRadius) {
    this->capsuleRadius = MeterToUnit(capsuleRadius);
}

float ComCharacterController::GetCapsuleHeight() const {
    return UnitToMeter(capsuleHeight);
}

void ComCharacterController::SetCapsuleHeight(const float capsuleHeight) {
    this->capsuleHeight = MeterToUnit(capsuleHeight);
}

float ComCharacterController::GetStepOffset() const {
    return UnitToMeter(stepOffset);
}

void ComCharacterController::SetStepOffset(const float stepOffset) {
    this->stepOffset = MeterToUnit(stepOffset);
}

float ComCharacterController::GetSlopeLimit() const {
    return RAD2DEG(Math::ACos(slopeDotZ));
}

void ComCharacterController::SetSlopeLimit(const float slopeLimit) {
    this->slopeDotZ = Math::Cos(DEG2RAD(slopeLimit));
}

BE_NAMESPACE_END
