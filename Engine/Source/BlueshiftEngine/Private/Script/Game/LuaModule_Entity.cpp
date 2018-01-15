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
#include "Script/LuaVM.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Components/ComTransform.h"
#include "Components/ComCollider.h"
#include "Components/ComBoxCollider.h"
#include "Components/ComSphereCollider.h"
#include "Components/ComCapsuleCollider.h"
#include "Components/ComConeCollider.h"
#include "Components/ComCylinderCollider.h"
#include "Components/ComMeshCollider.h"
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Components/ComConstantForce.h"
#include "Components/ComJoint.h"
#include "Components/ComFixedJoint.h"
#include "Components/ComHingeJoint.h"
#include "Components/ComSocketJoint.h"
#include "Components/ComSpringJoint.h"
#include "Components/ComCharacterJoint.h"
#include "Components/ComCharacterController.h"
#include "Components/ComCamera.h"
#include "Components/ComLight.h"
#include "Components/ComRenderable.h"
#include "Components/ComMeshRenderer.h"
#include "Components/ComStaticMeshRenderer.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Components/ComAnimator.h"
#include "Components/ComTextRenderer.h"
#include "Components/ComParticleSystem.h"
#include "Components/ComLogic.h"
#include "Components/ComScript.h"
#include "Components/ComSpline.h"
#include "Components/ComAudioListener.h"
#include "Components/ComAudioSource.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterEntity(LuaCpp::Module &module) {
    LuaCpp::Selector _Entity = module["Entity"];

    _Entity.SetClass<Entity>(module["Object"]);
    _Entity.AddClassMembers<Entity>(
        "name", &Entity::GetName,
        "tag", &Entity::GetTag,
        "layer", &Entity::GetLayer,
        "is_frozen", &Entity::IsFrozen,
        "is_active_self", &Entity::IsActiveSelf,
        "set_active", &Entity::SetActive,
        "game_world", &Entity::GetGameWorld,
        "parent", &Entity::GetParent,
        "set_parent", &Entity::SetParent,
        "children", &Entity::GetChildren,
        "find_child", &Entity::FindChild,
        "num_components", &Entity::NumComponents,
        "has_component", &Entity::HasComponent,
        "component_index", &Entity::GetComponentIndex,
        "component_by_index", static_cast<Component*(Entity::*)(int)const>(&Entity::GetComponent),
        "transform", static_cast<ComTransform*(Entity::*)()const>(&Entity::GetComponent<ComTransform>),
        "collider", static_cast<ComCollider*(Entity::*)()const>(&Entity::GetComponent<ComCollider>),
        "box_collider", static_cast<ComBoxCollider*(Entity::*)()const>(&Entity::GetComponent<ComBoxCollider>),
        "sphere_collider", static_cast<ComSphereCollider*(Entity::*)()const>(&Entity::GetComponent<ComSphereCollider>),
        "capsule_collider", static_cast<ComCapsuleCollider*(Entity::*)()const>(&Entity::GetComponent<ComCapsuleCollider>),
        "cone_collider", static_cast<ComConeCollider*(Entity::*)()const>(&Entity::GetComponent<ComConeCollider>),
        "cylinder_collider", static_cast<ComCylinderCollider*(Entity::*)()const>(&Entity::GetComponent<ComCylinderCollider>),
        "mesh_collider", static_cast<ComMeshCollider*(Entity::*)()const>(&Entity::GetComponent<ComMeshCollider>),
        "rigid_body", static_cast<ComRigidBody*(Entity::*)()const>(&Entity::GetComponent<ComRigidBody>),
        "sensor", static_cast<ComSensor*(Entity::*)()const>(&Entity::GetComponent<ComSensor>),
        "constant_force", static_cast<ComConstantForce*(Entity::*)()const>(&Entity::GetComponent<ComConstantForce>),
        "joint", static_cast<ComJoint*(Entity::*)()const>(&Entity::GetComponent<ComJoint>),
        "fixed_joint", static_cast<ComFixedJoint*(Entity::*)()const>(&Entity::GetComponent<ComFixedJoint>),
        "hinge_joint", static_cast<ComHingeJoint*(Entity::*)()const>(&Entity::GetComponent<ComHingeJoint>),
        "socket_joint", static_cast<ComSocketJoint*(Entity::*)()const>(&Entity::GetComponent<ComSocketJoint>),
        "spring_joint", static_cast<ComSpringJoint*(Entity::*)()const>(&Entity::GetComponent<ComSpringJoint>),
        "character_joint", static_cast<ComCharacterJoint*(Entity::*)()const>(&Entity::GetComponent<ComCharacterJoint>),
        "character_controller", static_cast<ComCharacterController*(Entity::*)()const>(&Entity::GetComponent<ComCharacterController>),
        "camera", static_cast<ComCamera*(Entity::*)()const>(&Entity::GetComponent<ComCamera>),
        "light", static_cast<ComLight*(Entity::*)()const>(&Entity::GetComponent<ComLight>),
        "renderable", static_cast<ComRenderable*(Entity::*)()const>(&Entity::GetComponent<ComRenderable>),
        "mesh_renderer", static_cast<ComMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComMeshRenderer>),
        "static_mesh_renderer", static_cast<ComStaticMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComStaticMeshRenderer>),
        "skinned_mesh_renderer", static_cast<ComSkinnedMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComSkinnedMeshRenderer>),
        "animator", static_cast<ComAnimator*(Entity::*)()const>(&Entity::GetComponent<ComAnimator>),
        "text_renderer", static_cast<ComTextRenderer*(Entity::*)()const>(&Entity::GetComponent<ComTextRenderer>),
        "particle_system", static_cast<ComParticleSystem*(Entity::*)()const>(&Entity::GetComponent<ComParticleSystem>),
        "audio_source", static_cast<ComAudioSource*(Entity::*)()const>(&Entity::GetComponent<ComAudioSource>),
        "audio_listener", static_cast<ComAudioListener*(Entity::*)()const>(&Entity::GetComponent<ComAudioListener>),
        "spline", static_cast<ComSpline*(Entity::*)()const>(&Entity::GetComponent<ComSpline>),
        "script", static_cast<ComScript*(Entity::*)()const>(&Entity::GetComponent<ComScript>),
        //"components", &Entity::GetComponents,
        "add_component", &Entity::AddComponent,
        "insert_component", &Entity::InsertComponent);

    _Entity["meta_object"] = Entity::metaObject;
    _Entity["destroy"].SetFunc(Entity::DestroyInstance);

    using EntityPtr = Entity*;
    using EntityPtrArray = Array<EntityPtr>;
    LuaCpp::Selector _EntityPtrList = module["EntityPtrArray"];
    _EntityPtrList.SetClass<EntityPtrArray>();
    _EntityPtrList.AddClassMembers<EntityPtrArray>(
        "at", static_cast<Entity *&(EntityPtrArray::*)(int)>(&EntityPtrArray::operator[]),
        "count", &EntityPtrArray::Count,
        "append", &EntityPtrArray::Append<EntityPtr &>,
        "insert", &EntityPtrArray::Insert<EntityPtr &>,
        "find_index", &EntityPtrArray::FindIndex<EntityPtr &>,
        "find", &EntityPtrArray::Find<EntityPtr &>,
        "remove_index", &EntityPtrArray::RemoveIndex,
        "remove", &EntityPtrArray::Remove<EntityPtr &>,
        "clear", &EntityPtrArray::Clear);
}

BE_NAMESPACE_END
