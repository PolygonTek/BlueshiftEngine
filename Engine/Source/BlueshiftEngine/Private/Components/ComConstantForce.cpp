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
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComConstantForce.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Constant Force", ComConstantForce, Component)
BEGIN_EVENTS(ComConstantForce)
END_EVENTS

void ComConstantForce::RegisterProperties() {
    REGISTER_PROPERTY("force", "Force", Vec3, force, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("torque", "Torque", Vec3, torque, Vec3::zero, "", PropertyInfo::EditorFlag);
}

ComConstantForce::ComConstantForce() {
}

ComConstantForce::~ComConstantForce() {
}

void ComConstantForce::Init() {
    // Mark as initialized
    SetInitialized(true);
}

void ComConstantForce::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    if (rigidBody) {
        rigidBody->GetBody()->ApplyCentralForce(force);
        rigidBody->GetBody()->ApplyTorque(torque);
    }
}

BE_NAMESPACE_END
