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

#include "../Precompiled.h"

// SIMD
#include "SIMD/SIMD.h"

// Common
#include "Core/Checksum_CRC32.h"
#include "Core/Checksum_MD5.h"
#include "Core/BinSearch.h"
#include "Core/Range.h"
#include "Core/Heap.h"
#include "Core/Allocator.h"
#include "Core/Str.h"
#include "Core/Variant.h"
#include "Core/Guid.h"
#include "Core/StrColor.h"
#include "Core/Delegate.h"

// Dynamic & Static Array
#include "Containers/Array.h"
#include "Containers/StaticArray.h"

// Math
#include "Math/Math.h"

// Containers
#include "Containers/Pair.h"
#include "Containers/Stack.h"
#include "Containers/LinkList.h"
#include "Containers/Queue.h"
#include "Containers/HashIndex.h"
#include "Containers/HashTable.h"
#include "Containers/StrArray.h"
#include "Containers/StrPool.h"
#include "Containers/HashMap.h"
#include "Containers/Hierarchy.h"

#include "Core/Timespan.h"
#include "Core/DateTime.h"
#include "Core/MinMaxCurve.h"

// Platform
#include "Platform/Intrinsics.h"
#include "Platform/cpuid.h"
#include "Platform/PlatformAtomic.h"
#include "Platform/PlatformProcess.h"
#include "Platform/PlatformFile.h"
#include "Platform/PlatformTime.h"
#include "Platform/PlatformThread.h"

// Platform Utils
#if defined(__IOS__)
#include "PlatformUtils/iOS/iOSDevice.h"
#elif defined(__ANDROID__)
#include "PlatformUtils/Android/AndroidJNI.h"
#include "PlatformUtils/Android/AndroidGPUInfo.h"
#endif

// Utils
#include "Core/ScopeLock.h"

// Profiler
#include "Profiler/Profiler.h"

// IO
#include "IO/File.h"
#include "IO/FileSystem.h"
#include "IO/ZipArchiver.h"

// Utils
#include "Core/ByteOrder.h"
#include "Core/Lexer.h"
#include "Core/Dict.h"
#include "Core/Expr.h"
#include "Core/CmdArgs.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"
#include "Core/Task.h"
#include "Core/Vertex.h"
#include "Core/JointPose.h"

// Introspection
#include "Core/Event.h"
#include "Core/Signal.h"
#include "Core/SignalObject.h"
#include "Core/Property.h"
#include "Core/Object.h"

// Image
#include "Image/Image.h"
#include "Image/DxtEncoder.h"
#include "Image/DxtDecoder.h"

// Sound
#include "Sound/Pcm.h"
#include "Sound/SoundSystem.h"

// RHI
#include "RHI/RHIOpenGL.h"

// Platform
#include "Platform/Platform.h"

// Input
#include "Input/KeyCodes.h"
#include "Input/KeyCmd.h"
#include "Input/InputSystem.h"

// Scripting
#include "Scripting/LuaVM.h"

// Render
#include "Render/Render.h"

// Physics
#include "Physics/Collider.h"
#include "Physics/Physics.h"

// Animation
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"

// Asset
#include "Asset/Asset.h"
#include "Asset/AssetImporter.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

// Components
#include "Components/Component.h"
#include "Components/Transform/ComTransform.h"
#include "Components/Transform/ComRectTransform.h"
#include "Components/Collider/ComCollider.h"
#include "Components/Collider/ComBoxCollider.h"
#include "Components/Collider/ComSphereCollider.h"
#include "Components/Collider/ComCapsuleCollider.h"
#include "Components/Collider/ComConeCollider.h"
#include "Components/Collider/ComCylinderCollider.h"
#include "Components/Collider/ComMeshCollider.h"
#include "Components/Joint/ComJoint.h"
#include "Components/Joint/ComFixedJoint.h"
#include "Components/Joint/ComHingeJoint.h"
#include "Components/Joint/ComSocketJoint.h"
#include "Components/Joint/ComSliderJoint.h"
#include "Components/Joint/ComSpringJoint.h"
#include "Components/Joint/ComWheelJoint.h"
#include "Components/Joint/ComCharacterJoint.h"
#include "Components/Renderable/ComRenderable.h"
#include "Components/Renderable/ComMeshRenderer.h"
#include "Components/Renderable/ComStaticMeshRenderer.h"
#include "Components/Renderable/ComSkinnedMeshRenderer.h"
#include "Components/Renderable/ComTextRenderer.h"
#include "Components/Renderable/ComParticleSystem.h"
#include "Components/Renderable/ComText.h"
#include "Components/Renderable/ComImage.h"
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Components/ComVehicleWheel.h"
#include "Components/ComConstantForce.h"
#include "Components/ComCharacterController.h"
#include "Components/ComCamera.h"
#include "Components/ComLight.h"
#include "Components/ComEnvironmentProbe.h"
#include "Components/ComAnimation.h"
#include "Components/ComAnimator.h"
#include "Components/ComSpline.h"
#include "Components/ComLogic.h"
#include "Components/ComSpline.h"
#include "Components/ComScript.h"
#include "Components/ComAudioListener.h"
#include "Components/ComAudioSource.h"
#include "Components/ComCanvas.h"

// GameSettings
#include "Game/GameSettings.h"

// GameWorld
#include "Game/Entity.h"
#include "Game/Prefab.h"
#include "Game/MapRenderSettings.h"
#include "Game/GameWorld.h"

#include "Engine/Common.h"
#include "Engine/Coords2D.h"
#include "Engine/Console.h"
#include "Engine/GameClient.h"
#include "Engine/Engine.h"
