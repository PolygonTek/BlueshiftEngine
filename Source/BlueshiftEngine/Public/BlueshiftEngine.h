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

// Common
#include "Core/Checksum_CRC32.h"
#include "Core/Checksum_MD5.h"
#include "Core/Range.h"
#include "Core/Heap.h"
#include "Core/Allocator.h"
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Core/Variant.h"
#include "Core/Guid.h"
#include "Core/StrColor.h"

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
#include "Containers/BinSearch.h"

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

// File
#include "File/File.h"
#include "File/FileMapping.h"
#include "File/FileSystem.h"
#include "File/ZipArchiver.h"

// Utils
#include "Core/ByteOrder.h"
#include "Core/Lexer.h"
#include "Core/WLexer.h"
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

// SIMD
#include "Simd/Simd.h"

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

// Script
#include "Script/LuaVM.h"

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
#include "Asset/GuidMapper.h"

// Components
#include "Components/Component.h"
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
#include "Components/ComReflectionProbe.h"
#include "Components/ComRenderable.h"
#include "Components/ComMeshRenderer.h"
#include "Components/ComStaticMeshRenderer.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Components/ComAnimator.h"
#include "Components/ComTextRenderer.h"
#include "Components/ComParticleSystem.h"
#include "Components/ComSpline.h"
#include "Components/ComLogic.h"
#include "Components/ComSpline.h"
#include "Components/ComScript.h"
#include "Components/ComAudioListener.h"
#include "Components/ComAudioSource.h"

// GameSettings
#include "Game/GameSettings.h"

// GameWorld
#include "Game/Entity.h"
#include "Game/Prefab.h"
#include "Game/MapRenderSettings.h"
#include "Game/GameWorld.h"

#include "Engine/Common.h"
#include "Engine/Console.h"
#include "Engine/GameClient.h"
#include "Engine/Engine.h"
