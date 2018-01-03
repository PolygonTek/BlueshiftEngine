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
#include "Asset/Asset.h"
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
#include "Components/ComTextRenderer.h"
#include "Components/ComParticleSystem.h"
#include "Components/ComLogic.h"
#include "Components/ComSpline.h"
#include "Components/ComScript.h"
#include "Components/ComAudioListener.h"
#include "Components/ComAudioSource.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterObject(LuaCpp::Module &module) {
    LuaCpp::Selector _MetaObject = module["MetaObject"];

    _MetaObject.SetClass<MetaObject>();
    _MetaObject.AddClassMembers<MetaObject>(
        "classname", &MetaObject::ClassName,
        "super_classname", &MetaObject::SuperClassName);

    LuaCpp::Selector _SignalObject = module["SignalObject"];
    
    _SignalObject.SetClass<SignalObject>();

    LuaCpp::Selector _Object = module["Object"];
    _Object.SetClass<Object>();
    _Object.AddClassMembers<Object>(
        "instance_id", &Object::GetInstanceID,
        "classname", &Object::ClassName,
        "super_classname", &Object::SuperClassName,
        "cast_entity", static_cast<Entity*(Object::*)()>(&Object::Cast<Entity>),
        "cast_transform", static_cast<ComTransform*(Object::*)()>(&Object::Cast<ComTransform>),
        "cast_collider", static_cast<ComCollider*(Object::*)()>(&Object::Cast<ComCollider>),
        "cast_box_collider", static_cast<ComBoxCollider*(Object::*)()>(&Object::Cast<ComBoxCollider>),
        "cast_sphere_collider", static_cast<ComSphereCollider*(Object::*)()>(&Object::Cast<ComSphereCollider>),
        "cast_capsule_collider", static_cast<ComCapsuleCollider*(Object::*)()>(&Object::Cast<ComCapsuleCollider>),
        "cast_cone_collider", static_cast<ComConeCollider*(Object::*)()>(&Object::Cast<ComConeCollider>),
        "cast_cylinder_collider", static_cast<ComCylinderCollider*(Object::*)()>(&Object::Cast<ComCylinderCollider>),
        "cast_mesh_collider", static_cast<ComMeshCollider*(Object::*)()>(&Object::Cast<ComMeshCollider>),
        "cast_rigid_body", static_cast<ComRigidBody*(Object::*)()>(&Object::Cast<ComRigidBody>),
        "cast_sensor", static_cast<ComSensor*(Object::*)()>(&Object::Cast<ComSensor>),
        "cast_constant_force", static_cast<ComConstantForce*(Object::*)()>(&Object::Cast<ComConstantForce>),
        "cast_joint", static_cast<ComJoint*(Object::*)()>(&Object::Cast<ComJoint>),
        "cast_fixed_joint", static_cast<ComFixedJoint*(Object::*)()>(&Object::Cast<ComFixedJoint>),
        "cast_hinge_joint", static_cast<ComHingeJoint*(Object::*)()>(&Object::Cast<ComHingeJoint>),
        "cast_socket_joint", static_cast<ComSocketJoint*(Object::*)()>(&Object::Cast<ComSocketJoint>),
        "cast_spring_joint", static_cast<ComSpringJoint*(Object::*)()>(&Object::Cast<ComSpringJoint>),
        "cast_character_joint", static_cast<ComCharacterJoint*(Object::*)()>(&Object::Cast<ComCharacterJoint>),
        "cast_character_controller", static_cast<ComCharacterController*(Object::*)()>(&Object::Cast<ComCharacterController>),
        "cast_camera", static_cast<ComCamera*(Object::*)()>(&Object::Cast<ComCamera>),
        "cast_light", static_cast<ComLight*(Object::*)()>(&Object::Cast<ComLight>),
        "cast_renderable", static_cast<ComRenderable*(Object::*)()>(&Object::Cast<ComRenderable>),
        "cast_mesh_renderer", static_cast<ComMeshRenderer*(Object::*)()>(&Object::Cast<ComMeshRenderer>),
        "cast_static_mesh_renderer", static_cast<ComStaticMeshRenderer*(Object::*)()>(&Object::Cast<ComStaticMeshRenderer>),
        "cast_skinned_mesh_renderer", static_cast<ComSkinnedMeshRenderer*(Object::*)()>(&Object::Cast<ComSkinnedMeshRenderer>),
        "cast_text_renderer", static_cast<ComTextRenderer*(Object::*)()>(&Object::Cast<ComTextRenderer>),
        "cast_particle_system", static_cast<ComParticleSystem*(Object::*)()>(&Object::Cast<ComParticleSystem>),
        "cast_audio_source", static_cast<ComAudioSource*(Object::*)()>(&Object::Cast<ComAudioSource>),
        "cast_audio_listener", static_cast<ComAudioListener*(Object::*)()>(&Object::Cast<ComAudioListener>),
        "cast_spline", static_cast<ComSpline*(Object::*)()>(&Object::Cast<ComSpline>),
        "cast_script", static_cast<ComScript*(Object::*)()>(&Object::Cast<ComScript>),
        "cast_texture_asset", static_cast<TextureAsset*(Object::*)()>(&Object::Cast<TextureAsset>), 
        "cast_shader_asset", static_cast<ShaderAsset*(Object::*)()>(&Object::Cast<ShaderAsset>),
        "cast_material_asset", static_cast<MaterialAsset*(Object::*)()>(&Object::Cast<MaterialAsset>),
        "cast_skeleton_asset", static_cast<SkeletonAsset*(Object::*)()>(&Object::Cast<SkeletonAsset>),
        "cast_mesh_asset", static_cast<MeshAsset*(Object::*)()>(&Object::Cast<MeshAsset>),
        "cast_anim_asset", static_cast<AnimAsset*(Object::*)()>(&Object::Cast<AnimAsset>),
        "cast_sound_asset", static_cast<SoundAsset*(Object::*)()>(&Object::Cast<SoundAsset>),
        "cast_prefab_asset", static_cast<PrefabAsset*(Object::*)()>(&Object::Cast<PrefabAsset>));

    _Object["meta_object"] = Object::metaObject;
    _Object["destroy"].SetFunc(&Object::DestroyInstance);
}

BE_NAMESPACE_END
