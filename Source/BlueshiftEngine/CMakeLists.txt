cmake_minimum_required(VERSION 2.8.12)

project(BlueshiftEngine)

configure_file(
    "${PROJECT_SOURCE_DIR}/Version.h.in"
    "${PROJECT_SOURCE_DIR}/Version.h")

set(ENGINE_FILES
    Precompiled.h
    Precompiled.cpp

    Public/BlueshiftEngine.h

    Public/Core/Allocator.h
    Public/Core/Checksum_CRC32.h
    Public/Core/Checksum_MD5.h
    Public/Core/Heap.h
    Public/Core/Range.h
    Public/Core/Str.h
    Public/Core/Variant.h
    Public/Core/VariantArg.h
    Public/Core/Guid.h
    Public/Core/WStr.h
    Public/Core/ByteOrder.h
    Public/Core/CmdArgs.h
    Public/Core/Cmds.h
    Public/Core/CVars.h
    Public/Core/Timespan.h
    Public/Core/DateTime.h
    Public/Core/MinMaxCurve.h
    Public/Core/Dict.h
    Public/Core/Expr.h
    Public/Core/Lexer.h
    Public/Core/WLexer.h
    Public/Core/Task.h
    Public/Core/Event.h
    Public/Core/Object.h
    Public/Core/Property.h
    Public/Core/Serializable.h
    Public/Core/Signal.h
    Public/Core/SignalObject.h
    Public/Core/DynamicAABBTree.h
    Public/Core/JointPose.h
    Public/Core/StrColor.h
    Public/Core/Vec4Color.h
    Public/Core/Vertex.h 

    Public/Containers/StaticArray.h
    Public/Containers/BinSearch.h
    Public/Containers/HashIndex.h
    Public/Containers/HashMap.h
    Public/Containers/HashTable.h
    Public/Containers/Hierarchy.h
    Public/Containers/LinkList.h
    Public/Containers/Array.h
    Public/Containers/Pair.h
    Public/Containers/Stack.h
    Public/Containers/Queue.h 
    Public/Containers/StrArray.h
    Public/Containers/StrPool.h
    Public/Containers/LinkedList.h

    Public/Image/Image.h 
    Public/Image/DxtCodec.h
    Public/Image/DxtDecoder.h
    Public/Image/DxtEncoder.h

    Public/Math/AABB.h
    Public/Math/Angles.h
    Public/Math/Complex.h
    Public/Math/Color.h
    Public/Math/Color3.h
    Public/Math/Color4.h
    Public/Math/CQuaternion.h
    Public/Math/Curve.h
    Public/Math/Hermite.h
    Public/Math/Cylinder.h
    Public/Math/FloatConverter.h
    Public/Math/Frustum.h
    Public/Math/Half.h
    Public/Math/Hammersley.h
    Public/Math/Math.h
    Public/Math/MathCommon.h
    Public/Math/Matrix.h
    Public/Math/Matrix2.h
    Public/Math/Matrix3.h
    Public/Math/Matrix3x4.h
    Public/Math/Matrix4.h
    Public/Math/OBB.h
    Public/Math/Plane.h
    Public/Math/Pluecker.h
    Public/Math/Point.h
    Public/Math/Quaternion.h
    Public/Math/Random.h
    Public/Math/Ray.h
    Public/Math/Rect.h
    Public/Math/RGBE9995.h
    Public/Math/Rotation.h
    Public/Math/Sphere.h
    Public/Math/SphericalHarmonics.h
    Public/Math/Vector.h
    Public/Math/Vector2.h
    Public/Math/Vector3.h
    Public/Math/Vector4.h

    Public/Simd/Simd.h
    Public/Simd/Simd_AVX.h
    Public/Simd/Simd_Generic.h
    Public/Simd/Simd_SSE4.h
    Public/Simd/smmintrin_emu.h
    Public/Simd/immintrin_emu.h
    Public/Simd/AVX/avx.h
    Public/Simd/AVX/avxb.h
    Public/Simd/AVX/avxf.h
    Public/Simd/AVX/avxi.h
    Public/Simd/SSE/sse.h
    Public/Simd/SSE/sseb.h
    Public/Simd/SSE/ssef.h
    Public/Simd/SSE/ssei.h

    Public/Platform/cpuid.h
    Public/Platform/Intrinsics.h
    Public/Platform/PlatformAtomic.h
    Public/Platform/PlatformProcess.h
    Public/Platform/PlatformFile.h
    Public/Platform/PlatformThread.h
    Public/Platform/PlatformTime.h
    Public/Platform/PlatformSystem.h

    Public/File/File.h
    Public/File/FileSystem.h
    Public/File/FileMapping.h
    Public/File/ZipArchiver.h
  
    Public/RHI/RHI.h
    Public/RHI/RHIOpenGL.h

    Public/Engine/Common.h
    Public/Engine/GameClient.h
    Public/Engine/Console.h
    Public/Engine/Engine.h

    Public/Asset/Asset.h
    Public/Asset/AssetImporter.h
    Public/Asset/GuidMapper.h

    Public/AnimController/AnimBlendTree.h
    Public/AnimController/AnimClip.h
    Public/AnimController/AnimLayer.h
    Public/AnimController/AnimState.h
    Public/AnimController/AnimController.h

    Public/Animator/Animator.h
    Public/Animator/AnimStateBlender.h

    Public/Components/ComAnimator.h
    Public/Components/ComAudioListener.h
    Public/Components/ComAudioSource.h
    Public/Components/ComBoxCollider.h
    Public/Components/ComCamera.h
    Public/Components/ComCapsuleCollider.h
    Public/Components/ComCharacterController.h
    Public/Components/ComCharacterJoint.h
    Public/Components/ComCollider.h
    Public/Components/ComConeCollider.h
    Public/Components/ComConstantForce.h
    Public/Components/ComCylinderCollider.h
    Public/Components/ComFixedJoint.h
    Public/Components/ComHingeJoint.h
    Public/Components/ComJoint.h
    Public/Components/ComLight.h
    Public/Components/ComReflectionProbe.h
    Public/Components/ComLogic.h
    Public/Components/ComScript.h
    Public/Components/ComMeshCollider.h
    Public/Components/ComMeshRenderer.h
    Public/Components/ComTextRenderer.h
    Public/Components/Component.h
    Public/Components/ComParticleSystem.h
    Public/Components/ComRenderable.h
    Public/Components/ComRigidBody.h
    Public/Components/ComSensor.h
    Public/Components/ComSkinnedMeshRenderer.h
    Public/Components/ComSocketJoint.h
    Public/Components/ComSphereCollider.h
    Public/Components/ComSpline.h
    Public/Components/ComSpringJoint.h
    Public/Components/ComStaticMeshRenderer.h
    Public/Components/ComTransform.h

    Public/Game/Entity.h
    Public/Game/Prefab.h
    Public/Game/MapRenderSettings.h
    Public/Game/GameWorld.h
    Public/Game/CastResult.h
    Public/Game/GameSettings.h
    Public/Game/TagLayerSettings.h
    Public/Game/PhysicsSettings.h
    Public/Game/PlayerSettings.h
    
    Public/Physics/Collider.h
    Public/Physics/Physics.h
    Public/Physics/PhysicsCollidable.h
    Public/Physics/PhysicsCollisionListener.h
    Public/Physics/PhysicsConstraint.h
    Public/Physics/PhysicsGenericConstraint.h
    Public/Physics/PhysicsGenericSpringConstraint.h
    Public/Physics/PhysicsHingeConstraint.h
    Public/Physics/PhysicsP2PConstraint.h
    Public/Physics/PhysicsRigidBody.h
    Public/Physics/PhysicsSensor.h
    Public/Physics/PhysicsSystem.h
    Public/Physics/PhysicsWorld.h

    Public/Sound/Pcm.h
    Public/Sound/SoundSystem.h

    Public/Render/Anim.h
    Public/Render/BufferCache.h
    Public/Render/Font.h
    Public/Render/ParticleMesh.h
    Public/Render/GuiMesh.h
    Public/Render/Material.h
    Public/Render/Mesh.h
    Public/Render/Render.h
    Public/Render/RenderSystem.h
    Public/Render/RenderContext.h  
    Public/Render/ParticleSystem.h
    Public/Render/ReflectionProbe.h
    Public/Render/SceneEntity.h
    Public/Render/SceneLight.h
    Public/Render/SceneView.h
    Public/Render/RenderWorld.h
    Public/Render/Shader.h
    Public/Render/Skeleton.h
    Public/Render/Skin.h
    Public/Render/SubMesh.h
    Public/Render/Texture.h  

    Public/Platform/Platform.h

    Public/Input/InputSystem.h
    Public/Input/KeyCmd.h
    Public/Input/KeyCodes.h

    Public/Script/LuaVM.h

    Private/Core/Checksum_CRC32.cpp
    Private/Core/Checksum_MD5.cpp
    Private/Core/Heap.cpp
    Private/Core/Str.cpp
    Private/Core/WStr.cpp
    Private/Core/Guid.cpp
    Private/Containers/HashIndex.cpp
    Private/Image/ImageInternal.h
    Private/Image/Image.cpp
    Private/Image/ImageColorSpace.cpp
    Private/Image/ImageConvert.cpp
    Private/Image/ImageCompressDXT.cpp
    Private/Image/ImageCompressETC.cpp
    Private/Image/ImageDecompressDXT.cpp
    Private/Image/ImageDecompressPVRTC.cpp
    Private/Image/ImageDecompressETC.cpp
    Private/Image/ImageFile.cpp
    Private/Image/ImageFileBMP.cpp
    Private/Image/ImageFileDDS.cpp
    Private/Image/ImageFilePVR.cpp
    Private/Image/ImageFileHDR.cpp
    Private/Image/ImageFileJPG.cpp
    Private/Image/ImageFilePCX.cpp
    Private/Image/ImageFilePNG.cpp
    Private/Image/ImageFileTGA.cpp
    Private/Image/ImageFormat.cpp
    Private/Image/ImageProcess.cpp
    Private/Image/ImageResize.cpp
    Private/Image/DXTDecoder.cpp
    Private/Image/DXTEncoder.cpp
    Private/Math/Vector3.cpp
    Private/Math/Vector4.cpp
    Private/Math/Color3.cpp
    Private/Math/Color4.cpp
    Private/Math/RGBE9995.cpp
    Private/Math/AABB.cpp
    Private/Math/Frustum.cpp
    Private/Math/Matrix4.cpp
    Private/Math/Quaternion.cpp
    Private/Math/Angles.cpp
    Private/Math/Half.cpp
    Private/Math/OBB.cpp
    Private/Math/Rect.cpp
    Private/Math/CQuaternion.cpp
    Private/Math/SphericalHarmonics.cpp
    Private/Math/Math.cpp
    Private/Math/Plane.cpp
    Private/Math/Rotation.cpp
    Private/Math/Complex.cpp
    Private/Math/Matrix2.cpp
    Private/Math/Matrix3x4.cpp
    Private/Math/Pluecker.cpp
    Private/Math/Sphere.cpp
    Private/Math/Cylinder.cpp
    Private/Math/Matrix3.cpp
    Private/Math/Point.cpp
    Private/Math/Vector2.cpp
    Private/SIMD/Simd.cpp
    Private/SIMD/Simd_AVX.cpp
    Private/SIMD/Simd_Generic.cpp
    Private/SIMD/Simd_SSE4.cpp
    Private/Platform/cpuid.cpp
    Private/Platform/PlatformBaseProcess.cpp
    Private/Platform/PlatformBaseFile.cpp
    Private/Platform/PlatformBaseThread.cpp
    Private/Platform/PlatformBaseTime.cpp
    Private/Platform/PlatformBaseSystem.cpp
    Private/File/File.cpp
    Private/File/FileSystem.cpp
    Private/File/FileMapping.cpp
    Private/File/ZipArchiver.cpp
    Private/Core/ByteOrder.cpp
    Private/Core/CmdArgs.cpp
    Private/Core/Cmds.cpp
    Private/Core/CVars.cpp
    Private/Core/Timespan.cpp
    Private/Core/DateTime.cpp
    Private/Core/Dict.cpp
    Private/Core/Expr.cpp
    Private/Core/MinMaxCurve.cpp
    Private/Core/Lexer.cpp
    Private/Core/WLexer.cpp
    Private/Core/Task.cpp
    Private/Core/Variant.cpp

    Private/RHIOpenGL/OpenGL/OpenGL.h
    Private/RHIOpenGL/OpenGL/OpenGL.cpp
    Private/RHIOpenGL/RGLInternal.h
    Private/RHIOpenGL/RGLBuffer.cpp
    Private/RHIOpenGL/RGLCommon.cpp
    Private/RHIOpenGL/RGLQuery.cpp
    Private/RHIOpenGL/RGLRenderTarget.cpp
    Private/RHIOpenGL/RGLShader.cpp
    Private/RHIOpenGL/RGLState.cpp
    Private/RHIOpenGL/RGLSync.cpp
    Private/RHIOpenGL/RGLTexture.cpp
    Private/RHIOpenGL/RGLVertexFormat.cpp

    Private/Engine/Common.cpp
    Private/Engine/GameClient.cpp
    Private/Engine/Console.cpp
    Private/Engine/Engine.cpp

    Private/Asset/Asset.cpp
    Private/Asset/FolderAsset.cpp
    Private/Asset/FbxAsset.cpp
    Private/Asset/TextureAsset.cpp
    Private/Asset/ShaderAsset.cpp
    Private/Asset/MaterialAsset.cpp
    Private/Asset/FontAsset.cpp
    Private/Asset/MeshAsset.cpp
    Private/Asset/ParticleSystemAsset.cpp
    Private/Asset/SkeletonAsset.cpp
    Private/Asset/AnimAsset.cpp
    Private/Asset/JointMaskAsset.cpp
    Private/Asset/AnimControllerAsset.cpp
    Private/Asset/ScriptAsset.cpp
    Private/Asset/PrefabAsset.cpp
    Private/Asset/SoundAsset.cpp
    Private/Asset/MapAsset.cpp
    Private/Asset/AssetImporter.cpp
    Private/Asset/GuidMapper.cpp
  
    Private/AnimController/AnimBlendTree.cpp
    Private/AnimController/AnimClip.cpp
    Private/AnimController/AnimLayer.cpp
    Private/AnimController/AnimState.cpp
    Private/AnimController/AnimController.cpp

    Private/Animator/Animator.cpp
    Private/Animator/AnimStateBlender.cpp
  
    Private/Components/ComAnimator.cpp
    Private/Components/ComAudioListener.cpp
    Private/Components/ComAudioSource.cpp
    Private/Components/ComBoxCollider.cpp  
    Private/Components/ComCamera.cpp
    Private/Components/ComCapsuleCollider.cpp
    Private/Components/ComCharacterController.cpp
    Private/Components/ComCharacterJoint.cpp
    Private/Components/ComCollider.cpp
    Private/Components/ComConeCollider.cpp
    Private/Components/ComConstantForce.cpp
    Private/Components/ComCylinderCollider.cpp
    Private/Components/ComFixedJoint.cpp
    Private/Components/ComHingeJoint.cpp
    Private/Components/ComJoint.cpp
    Private/Components/ComLight.cpp
    Private/Components/ComReflectionProbe.cpp
    Private/Components/ComLogic.cpp
    Private/Components/ComScript.cpp
    Private/Components/ComMeshCollider.cpp
    Private/Components/ComMeshRenderer.cpp
    Private/Components/ComTextRenderer.cpp
    Private/Components/Component.cpp
    Private/Components/ComParticleSystem.cpp
    Private/Components/ComRenderable.cpp
    Private/Components/ComRigidBody.cpp
    Private/Components/ComSensor.cpp
    Private/Components/ComSkinnedMeshRenderer.cpp
    Private/Components/ComSocketJoint.cpp
    Private/Components/ComSphereCollider.cpp
    Private/Components/ComSpline.cpp
    Private/Components/ComSpringJoint.cpp
    Private/Components/ComStaticMeshRenderer.cpp
    Private/Components/ComTransform.cpp

    Private/Game/Entity.cpp
    Private/Game/Prefab.cpp
    Private/Game/PrefabManager.cpp
    Private/Game/MapRenderSettings.cpp
    Private/Game/GameWorld.cpp
    Private/Game/CastResult.cpp  
    Private/Game/GameSettings.cpp
    Private/Game/TagLayerSettings.cpp
    Private/Game/PhysicsSettings.cpp
    Private/Game/PlayerSettings.cpp

    Private/Core/Event.cpp
    Private/Core/Object.cpp
    Private/Core/Serializable.cpp
    Private/Core/Signal.cpp
    Private/Core/SignalObject.cpp

    Private/Physics/ColliderInternal.h
    Private/Physics/PhysicsCVars.h
    Private/Physics/PhysicsInternal.h
    Private/Physics/Collider.cpp
    Private/Physics/ColliderManager.cpp
    Private/Physics/PhysicsCollidable.cpp
    Private/Physics/PhysicsCollisionListener.cpp
    Private/Physics/PhysicsConstraint.cpp
    Private/Physics/PhysicsCVars.cpp
    Private/Physics/PhysicsDebugDraw.cpp
    Private/Physics/PhysicsGenericConstraint.cpp
    Private/Physics/PhysicsGenericSpringConstraint.cpp
    Private/Physics/PhysicsHingeConstraint.cpp
    Private/Physics/PhysicsP2PConstraint.cpp
    Private/Physics/PhysicsRigidBody.cpp
    Private/Physics/PhysicsSensor.cpp
    Private/Physics/PhysicsSystem.cpp
    Private/Physics/PhysicsWorld.cpp

    Private/Sound/Pcm.cpp
    Private/Sound/Pcm_DecodeWav.cpp
    Private/Sound/Pcm_DecodeOgg.cpp
    Private/Sound/Sound.cpp
    Private/Sound/SoundSystem.cpp

    Private/Render/BModel.h
    Private/Render/Anim.cpp
    Private/Render/Anim_banim.cpp
    Private/Render/Anim_optimize.cpp
    Private/Render/AnimManager.cpp
    Private/Render/BufferCache.cpp
    Private/Render/ParticleMesh.cpp
    Private/Render/GuiMesh.cpp
    Private/Render/Material.cpp
    Private/Render/MaterialManager.cpp
    Private/Render/Mesh.cpp
    Private/Render/Mesh_bmesh.cpp
    Private/Render/Mesh_CreateMesh.cpp
    Private/Render/Mesh_SortAndMerge.cpp
    Private/Render/MeshManager.cpp
    Private/Render/RenderSystem.cpp
    Private/Render/RenderContext.cpp
    Private/Render/ParticleSystem.cpp
    Private/Render/ParticleSystemManager.cpp
    Private/Render/ReflectionProbe.cpp
    Private/Render/SceneEntity.cpp
    Private/Render/SceneLight.cpp
    Private/Render/SceneView.cpp
    Private/Render/RenderWorld.cpp
    Private/Render/RenderWorld_view.cpp
    Private/Render/Shader.cpp
    Private/Render/ShaderManager.cpp
    Private/Render/Skeleton.cpp
    Private/Render/SkeletonManager.cpp
    Private/Render/Skin.cpp
    Private/Render/SkinManager.cpp
    Private/Render/SubMesh.cpp
    Private/Render/Texture.cpp
    Private/Render/TextureManager.cpp
    Private/Render/FontFace.h
    Private/Render/Font.cpp
    Private/Render/FontManager.cpp
    Private/Render/FontFaceBitmap.cpp
    Private/Render/FontFaceFreeType.cpp
    Private/Render/DrawSurf.h
    Private/Render/FrameData.h
    Private/Render/RBackEnd.h
    Private/Render/RenderCmd.h
    Private/Render/RenderCVars.h
    Private/Render/RenderInternal.h
    Private/Render/RenderPostProcess.h
    Private/Render/RenderTarget.h
    Private/Render/RenderUtils.h
    Private/Render/Simplex.h
    Private/Render/VertexFormat.h
    Private/Render/FrameData.cpp
    Private/Render/RB_DebugTools.cpp
    Private/Render/RB_DrawSimple.cpp
    Private/Render/RB_Main.cpp
    Private/Render/RB_GenericPass.cpp
    Private/Render/RB_ShadowPass.cpp
    Private/Render/RB_ForwardBasePass.cpp
    Private/Render/RB_ForwardAdditivePass.cpp
    Private/Render/RB_PostProcess.cpp
    Private/Render/RB_Surf.cpp
    Private/Render/RB_Surf_Render.cpp
    Private/Render/RenderCVars.cpp
    Private/Render/RenderPostProcess.cpp
    Private/Render/RenderTarget.cpp
    Private/Render/RenderUtils.cpp
    Private/Render/VertexFormat.cpp  

    Private/Platform/Platform.cpp
    Private/Platform/PlatformGeneric.h
    Private/Platform/PlatformGeneric.cpp

    Private/Input/InputSystem.cpp
    Private/Input/KeyCmd.cpp

    Private/Script/LuaVM.cpp
    Private/Script/Math/LuaModule_Math.cpp
    Private/Script/Math/LuaModule_Complex.cpp
    Private/Script/Math/LuaModule_Vec2.cpp
    Private/Script/Math/LuaModule_Vec3.cpp
    Private/Script/Math/LuaModule_Vec4.cpp
    Private/Script/Math/LuaModule_Color3.cpp
    Private/Script/Math/LuaModule_Color4.cpp
    Private/Script/Math/LuaModule_Mat2.cpp
    Private/Script/Math/LuaModule_Mat3.cpp
    Private/Script/Math/LuaModule_Mat3x4.cpp
    Private/Script/Math/LuaModule_Mat4.cpp
    Private/Script/Math/LuaModule_Quaternion.cpp
    Private/Script/Math/LuaModule_Angles.cpp
    Private/Script/Math/LuaModule_Rotation.cpp
    Private/Script/Math/LuaModule_Plane.cpp
    Private/Script/Math/LuaModule_Sphere.cpp
    Private/Script/Math/LuaModule_Cylinder.cpp
    Private/Script/Math/LuaModule_AABB.cpp
    Private/Script/Math/LuaModule_OBB.cpp
    Private/Script/Math/LuaModule_Frustum.cpp
    Private/Script/Math/LuaModule_Ray.cpp
    Private/Script/Math/LuaModule_Point.cpp
    Private/Script/Math/LuaModule_Rect.cpp
    Private/Script/Engine/LuaModule_Common.cpp
    Private/Script/Input/LuaModule_InputSystem.cpp
    Private/Script/Screen/LuaModule_Screen.cpp
    Private/Script/Physics/LuaModule_Physics.cpp
    Private/Script/Core/LuaModule_Str.cpp
    Private/Script/Core/LuaModule_Object.cpp
    Private/Script/File/LuaModule_FileSystem.cpp
    Private/Script/File/LuaModule_File.cpp
    Private/Script/Asset/LuaModule_Asset.cpp
    Private/Script/Asset/LuaModule_TextureAsset.cpp
    Private/Script/Asset/LuaModule_ShaderAsset.cpp
    Private/Script/Asset/LuaModule_MaterialAsset.cpp
    Private/Script/Asset/LuaModule_SkeletonAsset.cpp
    Private/Script/Asset/LuaModule_MeshAsset.cpp
    Private/Script/Asset/LuaModule_AnimAsset.cpp
    Private/Script/Asset/LuaModule_AnimControllerAsset.cpp
    Private/Script/Asset/LuaModule_SoundAsset.cpp
    Private/Script/Asset/LuaModule_MapAsset.cpp
    Private/Script/Asset/LuaModule_PrefabAsset.cpp
    Private/Script/Components/LuaModule_Component.cpp
    Private/Script/Components/LuaModule_ComTransform.cpp
    Private/Script/Components/LuaModule_ComCollider.cpp
    Private/Script/Components/LuaModule_ComBoxCollider.cpp
    Private/Script/Components/LuaModule_ComSphereCollider.cpp
    Private/Script/Components/LuaModule_ComCylinderCollider.cpp
    Private/Script/Components/LuaModule_ComCapsuleCollider.cpp
    Private/Script/Components/LuaModule_ComMeshCollider.cpp
    Private/Script/Components/LuaModule_ComRigidBody.cpp
    Private/Script/Components/LuaModule_ComSensor.cpp
    Private/Script/Components/LuaModule_ComJoint.cpp
    Private/Script/Components/LuaModule_ComFixedJoint.cpp
    Private/Script/Components/LuaModule_ComHingeJoint.cpp
    Private/Script/Components/LuaModule_ComSocketJoint.cpp
    Private/Script/Components/LuaModule_ComSpringJoint.cpp
    Private/Script/Components/LuaModule_ComCharacterJoint.cpp
    Private/Script/Components/LuaModule_ComConstantForce.cpp
    Private/Script/Components/LuaModule_ComCharacterController.cpp
    Private/Script/Components/LuaModule_ComRenderable.cpp
    Private/Script/Components/LuaModule_ComMeshRenderer.cpp
    Private/Script/Components/LuaModule_ComStaticMeshRenderer.cpp
    Private/Script/Components/LuaModule_ComSkinnedMeshRenderer.cpp
    Private/Script/Components/LuaModule_ComAnimator.cpp
    Private/Script/Components/LuaModule_ComTextRenderer.cpp
    Private/Script/Components/LuaModule_ComParticleSystem.cpp
    Private/Script/Components/LuaModule_ComLight.cpp
    Private/Script/Components/LuaModule_ComCamera.cpp
    Private/Script/Components/LuaModule_ComAudioListener.cpp
    Private/Script/Components/LuaModule_ComAudioSource.cpp
    Private/Script/Components/LuaModule_ComSpline.cpp
    Private/Script/Components/LuaModule_ComScript.cpp
    Private/Script/Game/LuaModule_Entity.cpp
    Private/Script/Game/LuaModule_GameWorld.cpp

    Private/Core/DynamicAABBTree.cpp
    Private/Core/Vec4Color.cpp)

if (ANDROID)
    list(APPEND ENGINE_FILES
        Public/Platform/Posix/PlatformPosixProcess.h
        Public/Platform/Posix/PlatformPosixTime.h
        Public/Platform/Linux/PlatformLinuxProcess.h
        Public/Platform/Linux/PlatformLinuxTime.h
        Public/Platform/Android/PlatformAndroidProcess.h
        Public/Platform/Android/PlatformAndroidSystem.h
        Public/Platform/Android/PlatformAndroidThread.h
        Public/Platform/Android/PlatformAndroidFile.h

        Private/Platform/Posix/PlatformPosixProcess.cpp
        Private/Platform/Posix/PlatformPosixTime.cpp
        Private/Platform/Linux/PlatformLinuxProcess.cpp
        Private/Platform/Linux/PlatformLinuxTime.cpp
        Private/Platform/Android/PlatformAndroidProcess.cpp 
        Private/Platform/Android/PlatformAndroidSystem.cpp 
        Private/Platform/Android/PlatformAndroidThread.cpp
        Private/Platform/Android/PlatformAndroidFile.cpp
        Private/Platform/PlatformAndroid.h
        Private/Platform/PlatformAndroid.cpp

        Public/PlatformUtils/Android/AndroidJNI.h
        Public/PlatformUtils/Android/AndroidGPUInfo.h

        Private/PlatformUtils/Android/AndroidJNI.cpp
        Private/PlatformUtils/Android/AndroidGPUInfo.cpp

        Private/Sound/SoundSystemSLES.cpp
        Private/Sound/SoundBufferSLES.cpp
        Private/Sound/SoundSourceSLES.cpp)
elseif (WIN32)
    list(APPEND ENGINE_FILES
        Public/Platform/Windows/PlatformWinProcess.h
        Public/Platform/Windows/PlatformWinFile.h
        Public/Platform/Windows/PlatformWinThread.h
        Public/Platform/Windows/PlatformWinTime.h
        Public/Platform/Windows/PlatformWinSystem.h

        Private/Platform/Windows/PlatformWinProcess.cpp
        Private/Platform/Windows/PlatformWinFile.cpp
        Private/Platform/Windows/PlatformWinThread.cpp
        Private/Platform/Windows/PlatformWinTime.cpp
        Private/Platform/Windows/PlatformWinSystem.cpp
        Private/Platform/PlatformWin.h
        Private/Platform/PlatformWin.cpp

        Public/Network/Socket.h
        Public/Network/Packet.h
        Private/Network/Socket.cpp
        Private/Network/Packet.cpp)
    if (USE_WINDOWS_OPENAL)
        list(APPEND ENGINE_FILES
            Private/Sound/SoundSystemAL.cpp
            Private/Sound/SoundBufferAL.cpp
            Private/Sound/SoundSourceAL.cpp)
    else ()
        list(APPEND ENGINE_FILES
            Private/Sound/SoundSystemDS.cpp
            Private/Sound/SoundBufferDS.cpp
            Private/Sound/SoundSourceDS.cpp)
    endif ()
elseif (UNIX)
    list(APPEND ENGINE_FILES
        Public/Platform/Posix/PlatformPosixProcess.h
        Public/Platform/Posix/PlatformPosixFile.h
        Public/Platform/Posix/PlatformPosixThread.h
        Public/Platform/Posix/PlatformPosixTime.h
        Public/Platform/Posix/PlatformPosixSystem.h

        Private/Platform/Posix/PlatformPosixProcess.cpp
        Private/Platform/Posix/PlatformPosixFile.cpp
        Private/Platform/Posix/PlatformPosixThread.cpp
        Private/Platform/Posix/PlatformPosixTime.cpp
        Private/Platform/Posix/PlatformPosixSystem.cpp

        Private/Sound/SoundSystemAL.cpp
        Private/Sound/SoundBufferAL.cpp
        Private/Sound/SoundSourceAL.cpp)

    if (APPLE)
        list(APPEND ENGINE_FILES
            Public/Platform/Apple/PlatformAppleTime.h
            Public/Platform/Apple/PlatformAppleProcess.h
            Public/Platform/Apple/PlatformAppleSystem.h

            Private/Platform/Apple/PlatformAppleTime.cpp
            Private/Platform/Apple/PlatformAppleProcess.mm
            Private/Platform/Apple/PlatformAppleSystem.mm)
    
        if (IOS)
            list(APPEND ENGINE_FILES
                Public/Platform/IOS/PlatformIOSFile.h
                Public/Platform/IOS/PlatformIOSProcess.h
                Public/Platform/IOS/PlatformIOSSystem.h

                Private/Platform/IOS/PlatformIOSFile.mm
                Private/Platform/IOS/PlatformIOSProcess.mm
                Private/Platform/IOS/PlatformIOSSystem.mm
                Private/Platform/PlatformIOS.h
                Private/Platform/PlatformIOS.mm

                Public/PlatformUtils/iOS/iOSDevice.h
                Private/PlatformUtils/iOS/iOSDevice.mm)
        else ()
            list(APPEND ENGINE_FILES
                Public/Platform/MacOS/PlatformMacOSProcess.h
                Public/Platform/MacOS/PlatformMacOSSystem.h

                Private/Platform/MacOS/PlatformMacOSProcess.mm
                Private/Platform/MacOS/PlatformMacOSSystem.mm
                Private/Platform/PlatformMacOS.h
                Private/Platform/PlatformMacOS.mm

                Public/Network/Socket.h
                Public/Network/Packet.h
                Private/Network/Socket.cpp
                Private/Network/Packet.cpp)
        endif ()
    endif ()
else ()
    list(APPEND ENGINE_FILES
        Public/Platform/Linux/PlatformLinuxProcess.h
        Private/Platform/Linux/PlatformLinuxProcess.cpp)
endif ()

if (XAMARIN AND NOT WIN32)
    list(APPEND RENDERER_FILES
        Private/RHIOpenGL/OpenGL/OpenGLES3.h
        Private/RHIOpenGL/OpenGL/OpenGLES3.cpp
        Private/RHIOpenGL/OpenGL/XamarinOpenGL.h
        Private/RHIOpenGL/OpenGL/XamarinOpenGL.cpp
        Private/RHIOpenGL/RGLPlatformXamarin.cpp)
elseif (ANDROID)
    list(APPEND RENDERER_FILES
        Private/RHIOpenGL/OpenGL/OpenGLES3.h
        Private/RHIOpenGL/OpenGL/OpenGLES3.cpp
        Private/RHIOpenGL/OpenGL/AndroidOpenGL.h
        Private/RHIOpenGL/OpenGL/AndroidOpenGL.cpp
        Private/RHIOpenGL/RGLPlatformAndroid.cpp)
elseif (WIN32)
    if (USE_DESKTOP_EGL)
        list(APPEND RENDERER_FILES
            Private/RHIOpenGL/OpenGL/OpenGLES3.h
            Private/RHIOpenGL/OpenGL/OpenGLES3.cpp
            Private/RHIOpenGL/OpenGL/WinOpenGL.h
            Private/RHIOpenGL/RGLPlatformWin.cpp)
    else ()
        list(APPEND RENDERER_FILES
            Private/RHIOpenGL/OpenGL/OpenGL3.h
            Private/RHIOpenGL/OpenGL/OpenGL3.cpp
            Private/RHIOpenGL/OpenGL/WinOpenGL.h
            Private/RHIOpenGL/RGLPlatformWin.cpp)
    endif ()
elseif (APPLE)
    if (IOS)
        list(APPEND RENDERER_FILES
            Private/RHIOpenGL/OpenGL/OpenGLES3.h
            Private/RHIOpenGL/OpenGL/OpenGLES3.cpp
            Private/RHIOpenGL/OpenGL/IOSOpenGL.h
            Private/RHIOpenGL/OpenGL/IOSOpenGL.mm
            Private/RHIOpenGL/RGLPlatformIOS.mm)
    else ()
        list(APPEND RENDERER_FILES
            Private/RHIOpenGL/OpenGL/OpenGL3.h
            Private/RHIOpenGL/OpenGL/OpenGL3.cpp
            Private/RHIOpenGL/OpenGL/MacOSOpenGL.h
            Private/RHIOpenGL/RGLPlatformMacOS.mm)
    endif ()
endif ()

if (IOS)
    set(GGL_FILES
        Private/RHIOpenGL/OpenGL/GGL/ggles3.c
        Private/RHIOpenGL/OpenGL/GGL/ggles3.h)
elseif (ANDROID OR (WIN32 AND USE_DESKTOP_EGL))
    set(GGL_FILES
        Private/RHIOpenGL/OpenGL/GGL/ggles3.c
        Private/RHIOpenGL/OpenGL/GGL/ggles3.h
        Private/RHIOpenGL/OpenGL/GGL/gegl.c
        Private/RHIOpenGL/OpenGL/GGL/gegl.h)
else ()
    set(GGL_FILES
        Private/RHIOpenGL/OpenGL/GGL/gglcore32.c
        Private/RHIOpenGL/OpenGL/GGL/gglcore32.h)

    if (WIN32)
        list(APPEND GGL_FILES
            Private/RHIOpenGL/OpenGL/GGL/gwgl.c
            Private/RHIOpenGL/OpenGL/GGL/gwgl.h)
    endif ()
endif ()

set(ALL_FILES
    ${ENGINE_FILES}
    ${RENDERER_FILES}
    ${GGL_FILES}
)

auto_source_group(${ALL_FILES})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}/BlueshiftEngine/Public
    ${ENGINE_INCLUDE_DIR}/ThirdParty
    ${ENGINE_INCLUDE_DIR}/ThirdParty/asio/include
)

enable_precompiled_header(Precompiled.h Precompiled.cpp ENGINE_FILES RENDERER_FILES)

if (ANDROID)
    add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

    target_link_libraries(${PROJECT_NAME}
        android 
        atomic 
        log 
        EGL 
        GLESv3 
        OpenSLES)
else ()
    add_library(${PROJECT_NAME} STATIC ${ALL_FILES})
endif ()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++0x"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK "NO"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
        #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS NO
        #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug] YES
        #XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${CMAKE_CURRENT_LIST_DIR}/Precompiled.h
        #XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
        XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99") 

    if (WITHDEBUG)
        set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP NO)
    else ()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_COPY_PHASE_STRIP YES
            XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] NO)
    endif ()

    if (IOS)
        include_directories(${ENGINE_INCLUDE_DIR}/ThirdParty/OpenGL/include)

        set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET}
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # Universal (iPad + iPhone)

        add_framework(${PROJECT_NAME} Foundation)
        add_framework(${PROJECT_NAME} UIKit)
        add_framework(${PROJECT_NAME} OpenGLES)
        add_framework(${PROJECT_NAME} CoreGraphics)
        add_framework(${PROJECT_NAME} QuartzCore)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_framework(${PROJECT_NAME} Carbon)
        add_framework(${PROJECT_NAME} Foundation)
        add_framework(${PROJECT_NAME} AppKit)
        add_framework(${PROJECT_NAME} OpenGL)
        add_framework(${PROJECT_NAME} CoreGraphics)
        add_framework(${PROJECT_NAME} CoreVideo)
    endif ()

    add_framework(${PROJECT_NAME} OpenAL)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)

    if (USE_DESKTOP_EGL)
        include(FindOpenGLES3)
        include(FindEGL)

        include_directories(${ENGINE_INCLUDE_DIR}/ThirdParty/OpenGL/include)

        find_library(OPENGLES3_LIBRARY GLESv2)
        find_library(EGL_LIBRARY EGL)

        target_link_libraries(${PROJECT_NAME} ${EGL_LIBRARY} ${OPENGLES3_LIBRARY})
    else ()
        find_package(OpenGL REQUIRED)

        target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
    endif ()

    if (USE_WINDOWS_OPENAL)
        include(FindOpenAL)
    
        target_include_directories(${PROJECT_NAME} PUBLIC ${OPENAL_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
    else ()
        target_link_libraries(${PROJECT_NAME} dxguid dsound)
    endif ()
endif ()

target_link_libraries(${PROJECT_NAME} 
    zlib
    minizip
    libjpeg
    libpng
    libpvrt
    etcpack_lib
    Etc2CompLib
    libogg
    libvorbis
    jsoncpp
    tinyxml2
    freetype
    Bullet
    HACD
    luasocket
    LuaCpp
    NvTriStrip)

if (USE_LUAJIT)
    target_include_directories(${PROJECT_NAME} PUBLIC ${ENGINE_INCLUDE_DIR}/ThirdParty/luaJIT/src)

    include(FindLuaJIT)
    target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARY})
else ()    
    target_include_directories(${PROJECT_NAME} PUBLIC ${ENGINE_INCLUDE_DIR}/ThirdParty/lua)

    target_link_libraries(${PROJECT_NAME} lua)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "Blueshift"
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
