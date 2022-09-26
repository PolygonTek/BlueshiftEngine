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
#include "Scripting/LuaVM.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Components/ComTransform.h"
#include "Components/ComRectTransform.h"
#include "Components/ComCollider.h"
#include "Components/ComBoxCollider.h"
#include "Components/ComSphereCollider.h"
#include "Components/ComCapsuleCollider.h"
#include "Components/ComConeCollider.h"
#include "Components/ComCylinderCollider.h"
#include "Components/ComMeshCollider.h"
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Components/ComVehicleWheel.h"
#include "Components/ComConstantForce.h"
#include "Components/ComJoint.h"
#include "Components/ComFixedJoint.h"
#include "Components/ComHingeJoint.h"
#include "Components/ComSocketJoint.h"
#include "Components/ComSliderJoint.h"
#include "Components/ComSpringJoint.h"
#include "Components/ComWheelJoint.h"
#include "Components/ComCharacterJoint.h"
#include "Components/ComCharacterController.h"
#include "Components/ComCamera.h"
#include "Components/ComCanvas.h"
#include "Components/ComLight.h"
#include "Components/ComRenderable.h"
#include "Components/ComMeshRenderer.h"
#include "Components/ComStaticMeshRenderer.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Components/ComAnimation.h"
#include "Components/ComAnimator.h"
#include "Components/ComTextRenderer.h"
#include "Components/ComText.h"
#include "Components/ComImage.h"
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
        "is_active_self", &Entity::IsActiveSelf,
        "is_active_in_hierarchy", &Entity::IsActiveInHierarchy,
        "set_active", &Entity::SetActive,
        "game_world", &Entity::GetGameWorld,
        "parent", &Entity::GetParent,
        "set_parent", &Entity::SetParent,
        "children", &Entity::GetChildren,
        "children_recursive", &Entity::GetChildrenRecursive,
        "find_child", &Entity::FindChild,
        "local_aabb", &Entity::GetLocalAABB,
        "world_aabb", &Entity::GetWorldAABB,
        "ray_cast_rect", &Entity::RayCastRect,
        "num_components", &Entity::NumComponents,
        "has_component", &Entity::HasComponent,
        "component_index", &Entity::GetComponentIndex,
        "component_by_index", static_cast<Component*(Entity::*)(int)const>(&Entity::GetComponent),
        "components_in_children", static_cast<ComponentPtrArray(Entity::*)(const MetaObject *, bool)const>(&Entity::GetComponentsInChildren),
        "components", static_cast<ComponentPtrArray(Entity::*)(const MetaObject *)const>(&Entity::GetComponents),
        "transform", static_cast<ComTransform*(Entity::*)()const>(&Entity::GetComponent<ComTransform>),
        "rect_transform", static_cast<ComRectTransform*(Entity::*)()const>(&Entity::GetComponent<ComRectTransform>),
        "collider", static_cast<ComCollider*(Entity::*)()const>(&Entity::GetComponent<ComCollider>),
        "box_collider", static_cast<ComBoxCollider*(Entity::*)()const>(&Entity::GetComponent<ComBoxCollider>),
        "sphere_collider", static_cast<ComSphereCollider*(Entity::*)()const>(&Entity::GetComponent<ComSphereCollider>),
        "capsule_collider", static_cast<ComCapsuleCollider*(Entity::*)()const>(&Entity::GetComponent<ComCapsuleCollider>),
        "cone_collider", static_cast<ComConeCollider*(Entity::*)()const>(&Entity::GetComponent<ComConeCollider>),
        "cylinder_collider", static_cast<ComCylinderCollider*(Entity::*)()const>(&Entity::GetComponent<ComCylinderCollider>),
        "mesh_collider", static_cast<ComMeshCollider*(Entity::*)()const>(&Entity::GetComponent<ComMeshCollider>),
        "rigid_body", static_cast<ComRigidBody*(Entity::*)()const>(&Entity::GetComponent<ComRigidBody>),
        "sensor", static_cast<ComSensor*(Entity::*)()const>(&Entity::GetComponent<ComSensor>),
        "vehicle_wheel", static_cast<ComVehicleWheel*(Entity::*)()const>(&Entity::GetComponent<ComVehicleWheel>),
        "constant_force", static_cast<ComConstantForce*(Entity::*)()const>(&Entity::GetComponent<ComConstantForce>),
        "joint", static_cast<ComJoint*(Entity::*)()const>(&Entity::GetComponent<ComJoint>),
        "fixed_joint", static_cast<ComFixedJoint*(Entity::*)()const>(&Entity::GetComponent<ComFixedJoint>),
        "hinge_joint", static_cast<ComHingeJoint*(Entity::*)()const>(&Entity::GetComponent<ComHingeJoint>),
        "socket_joint", static_cast<ComSocketJoint*(Entity::*)()const>(&Entity::GetComponent<ComSocketJoint>),
        "slider_joint", static_cast<ComSliderJoint*(Entity::*)()const>(&Entity::GetComponent<ComSliderJoint>),
        "spring_joint", static_cast<ComSpringJoint*(Entity::*)()const>(&Entity::GetComponent<ComSpringJoint>),
        "wheel_joint", static_cast<ComWheelJoint*(Entity::*)()const>(&Entity::GetComponent<ComWheelJoint>),
        "character_joint", static_cast<ComCharacterJoint*(Entity::*)()const>(&Entity::GetComponent<ComCharacterJoint>),
        "character_controller", static_cast<ComCharacterController*(Entity::*)()const>(&Entity::GetComponent<ComCharacterController>),
        "camera", static_cast<ComCamera*(Entity::*)()const>(&Entity::GetComponent<ComCamera>),
        "canvas", static_cast<ComCanvas*(Entity::*)()const>(&Entity::GetComponent<ComCanvas>),
        "light", static_cast<ComLight*(Entity::*)()const>(&Entity::GetComponent<ComLight>),
        "renderable", static_cast<ComRenderable*(Entity::*)()const>(&Entity::GetComponent<ComRenderable>),
        "mesh_renderer", static_cast<ComMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComMeshRenderer>),
        "static_mesh_renderer", static_cast<ComStaticMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComStaticMeshRenderer>),
        "skinned_mesh_renderer", static_cast<ComSkinnedMeshRenderer*(Entity::*)()const>(&Entity::GetComponent<ComSkinnedMeshRenderer>),
        "animation", static_cast<ComAnimation*(Entity::*)()const>(&Entity::GetComponent<ComAnimation>),
        "animator", static_cast<ComAnimator*(Entity::*)()const>(&Entity::GetComponent<ComAnimator>),
        "text_renderer", static_cast<ComTextRenderer*(Entity::*)()const>(&Entity::GetComponent<ComTextRenderer>),
        "text", static_cast<ComText*(Entity::*)()const>(&Entity::GetComponent<ComText>),
        "image", static_cast<ComImage*(Entity::*)()const>(&Entity::GetComponent<ComImage>),
        "particle_system", static_cast<ComParticleSystem*(Entity::*)()const>(&Entity::GetComponent<ComParticleSystem>),
        "audio_source", static_cast<ComAudioSource*(Entity::*)()const>(&Entity::GetComponent<ComAudioSource>),
        "audio_listener", static_cast<ComAudioListener*(Entity::*)()const>(&Entity::GetComponent<ComAudioListener>),
        "spline", static_cast<ComSpline*(Entity::*)()const>(&Entity::GetComponent<ComSpline>),
        "script", static_cast<ComScript*(Entity::*)()const>(&Entity::GetComponent<ComScript>),
        "nth_script", static_cast<ComScript*(Entity::*)(int)const>(&Entity::GetComponent<ComScript>),
        "add_new_component", &Entity::AddNewComponent);

    _Entity["meta_object"] = Entity::metaObject;
    _Entity["destroy"].SetFunc(Entity::DestroyInstance);

    using EntityPtr = Entity*;
    using EntityPtrArray = Array<EntityPtr>;

    LuaCpp::Selector _EntityPtrArray = module["EntityPtrArray"];
    _EntityPtrArray.SetClass<EntityPtrArray>();
    _EntityPtrArray.AddClassMembers<EntityPtrArray>(
        "at", static_cast<EntityPtr&(EntityPtrArray::*)(int)>(&EntityPtrArray::operator[]),
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
