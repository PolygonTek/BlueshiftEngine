cmake_minimum_required(VERSION 3.5)

project(TestD3D12)

set(SRC_FILES
    Precompiled.h
    Precompiled.cpp
    Profiler.h
    App.h
    App.cpp
    App_ImGui.cpp
    LoadingSystem.h
    LoadingSystem.cpp
    Scene/GameObject.h
    Scene/GameObject.cpp
    Scene/GameLight.h
    Scene/GameLight.cpp
    Scene/Scene.h
    Scene/SceneRegistry.h
    Scene/SceneRegistry.cpp
    Scene/SceneMatrix.h
    Scene/SceneMatrix.cpp
    Scene/SceneMaterialBall.h
    Scene/SceneMaterialBall.cpp
    Scene/SceneDebugDraw.h
    Scene/SceneDebugDraw.cpp
    Scene/SceneOcean.h
    Scene/SceneOcean.cpp
    WinMain.cpp
    WinResource.h
    WinRes/small.ico
    WinRes/TestD3D12.ico
    WinRes/TestD3D12.rc
    RHI/RHI.h
    RHI/RHI.cpp
    RHI/D3D12/D3D12Common.h
    RHI/D3D12/D3D12Renderer.h
    RHI/D3D12/D3D12Renderer.cpp
    RHI/D3D12/D3D12SwapChain.h
    RHI/D3D12/D3D12SwapChain.cpp
    RHI/D3D12/D3D12DescriptorPool.h
    RHI/D3D12/D3D12DescriptorPool.cpp
    RHI/D3D12/D3D12ShaderVisibleDescriptorHeap.h
    RHI/D3D12/D3D12ShaderVisibleDescriptorHeap.cpp
    RHI/D3D12/D3D12Buffer.h
    RHI/D3D12/D3D12Buffer.cpp
    RHI/D3D12/D3D12Shader.h
    RHI/D3D12/D3D12Shader.cpp
    RHI/D3D12/D3D12Texture.h
    RHI/D3D12/D3D12Texture.cpp
    RHI/D3D12/D3D12Sampler.h
    RHI/D3D12/D3D12Sampler.cpp
    RHI/D3D12/D3D12VertexBuffer.h
    RHI/D3D12/D3D12VertexBuffer.cpp
    RHI/D3D12/D3D12IndexBuffer.h
    RHI/D3D12/D3D12IndexBuffer.cpp
    RHI/D3D12/D3D12ConstantBuffer.h
    RHI/D3D12/D3D12ConstantBuffer.cpp
    RHI/D3D12/D3D12PipelineState.h
    RHI/D3D12/D3D12PipelineState.cpp
    RHI/D3D12/D3D12RootBindingData.h
    RHI/D3D12/D3D12RootBindingData.cpp
    RHI/D3D12/D3D12ResourceRequest.h
    RHI/D3D12/D3D12ResourceRequest.cpp
    RHI/D3D12/D3D12Query.h
    RHI/D3D12/D3D12Query.cpp
    RHI/D3D12/D3D12CommandList.h
    RHI/D3D12/D3D12CommandList.cpp
    RHI/D3D12/D3D12CommandListPool.h
    RHI/D3D12/D3D12CommandListPool.cpp
    RHI/D3D12/D3D12FrameThreadData.h
    RHI/D3D12/D3D12FrameThreadData.cpp
    RHI/D3D12/D3D12ImGuiRenderer.h
    RHI/D3D12/D3D12ImGuiRenderer.cpp
    RHI/D3D12/D3D12ImGuiOverlay.h
    RHI/D3D12/D3D12ImGuiOverlay.cpp
    Render/Animation/Anim.h
    Render/Animation/Anim.cpp
    Render/Animation/Anim_InternalData.cpp
    Render/Animation/AnimManager.h
    Render/Animation/AnimManager.cpp
    Render/Animation/SkinningJointCache.h
    Render/Animation/SkinningJointCache.cpp
    Render/Animation/Skeleton.h
    Render/Animation/Skeleton.cpp
    Render/Animation/SkeletonManager.h
    Render/Animation/SkeletonManager.cpp
    Render/Font/Font.h
    Render/Font/Font.cpp
    Render/Font/FontFace.h
    Render/Font/FontFaceBitmap.cpp
    Render/Font/FontFaceTrueType.cpp
    Render/Font/FontManager.h
    Render/Font/FontManager.cpp
    Render/Font/FontFile.h
    Render/Font/TrueTypeFont.h
    Render/Font/TrueTypeFont.cpp
    Render/Material/Material.h
    Render/Material/Material.cpp
    Render/Material/MaterialManager.h
    Render/Material/MaterialManager.cpp
    Render/Mesh/BModel.h
    Render/Mesh/Mesh.h
    Render/Mesh/Mesh.cpp
    Render/Mesh/Mesh_InternalData.cpp
    Render/Mesh/par_octasphere.h
    Render/Mesh/Mesh_Create.cpp
    Render/Mesh/MeshInstance.h
    Render/Mesh/MeshInstance.cpp
    Render/Mesh/MeshManager.h
    Render/Mesh/MeshManager.cpp
    Render/Mesh/SubMesh.h
    Render/Mesh/SubMesh.cpp
    Render/Mesh/GuiMesh.h
    Render/Mesh/GuiMesh.cpp
    Render/ShaderPipeline/ShaderPipelineCache.h
    Render/ShaderPipeline/ShaderPipelineCache.cpp
    Render/ShaderPipeline/ShaderFeatureSet.h
    Render/ShaderPipeline/ShaderFeatureSet.cpp
    Render/ShaderPipeline/ShaderPermutationKey.h
    Render/ShaderPipeline/ShaderPermutationKey.cpp
    Render/ShaderPipeline/ShaderPipeline.h
    Render/ShaderPipeline/ShaderPipeline.cpp
    Render/ShaderPipeline/ShaderTemplate.h
    Render/ShaderPipeline/ShaderTemplate.cpp
    Render/ShaderPipeline/ShaderTemplateManager.h
    Render/ShaderPipeline/ShaderTemplateManager.cpp
    Render/ShaderPipeline/VertexInputLayoutFactory.h
    Render/ShaderPipeline/VertexInputLayoutFactory.cpp
    Render/Texture/TextureStreamingFeedback.h
    Render/Texture/TextureStreamingFeedback.cpp
    Render/Texture/Texture.h
    Render/Texture/Texture.cpp
    Render/Texture/TextureManager.h
    Render/Texture/TextureManager.cpp
    Render/RenderCapture.h
    Render/RenderCapture.cpp
    Render/RenderCvars.h
    Render/RenderCvars.cpp
    Render/ColorGradingLUTGenerator.h
    Render/ColorGradingLUTGenerator.cpp
    Render/ConvolutionBloomKernelGenerator.h
    Render/ConvolutionBloomKernelGenerator.cpp
    Render/RenderWork/ConvolutionBloomKernelRenderWork.h
    Render/RenderWork/ConvolutionBloomKernelRenderWork.cpp
    Render/IBLBaker.h
    Render/IBLBaker.cpp
    Render/ProbeBakeJob.h
    Render/ProbeBakeJob.cpp
    Render/RenderWork/ProbeBakeRenderWork.h
    Render/RenderWork/ProbeBakeRenderWork.cpp
    Render/ProbeBakeSystem.h
    Render/ProbeBakeSystem.cpp
    Render/GpuTiming.h
    Render/GpuTiming.cpp
    Render/ShaderPipelineBootstrap.h
    Render/ShaderPipelineBootstrap.cpp
    Render/RenderContext.h
    Render/RenderContext.cpp
    Render/RenderFrameData.h
    Render/RenderFrameData.cpp
    Render/RenderWork/RenderWork.h
    Render/RenderWork/RenderWorkQueue.h
    Render/RenderWork/RenderWorkQueue.cpp
    Render/GpuFFT.h
    Render/GpuFFT.cpp
    Render/GpuOcean.h
    Render/GpuOceanSettings.cpp
    Render/GpuOceanSurface.cpp
    Render/GpuOcean.cpp
    Render/GpuSphericalHarmonics.h
    Render/GpuSphericalHarmonics.cpp
    Render/ShaderPacking.h
    Render/ShaderMaterialTable.h
    Render/ShaderMaterialTable.cpp
    Render/ShaderLightTable.h
    Render/ShaderLightTable.cpp
    Render/ShaderLightCullingTable.h
    Render/ShaderLightCullingTable.cpp
    Render/ShaderSurfaceTable.h
    Render/ShaderSurfaceTable.cpp
    Render/RenderSystem.h
    Render/RenderSystem.cpp
    Render/DebugDrawPipeline.h
    Render/DebugDrawVertex.h
    Render/Backend/RenderBackend.h
    Render/Backend/RenderBackend.cpp
    Render/Backend/RenderBackend_HiZCulling.cpp
    Render/Backend/RenderBackend_GenHiZ.cpp
    Render/Backend/RenderBackend_HiZDebug.cpp
    Render/Backend/RenderBackend_PostProcess.cpp
    Render/Backend/RenderBackend_DebugDraw.cpp
    Render/Backend/RenderBackend_DrawGenericSurf.cpp
    Render/Backend/RenderBackend_DrawSkySurf.cpp
    Render/Frontend/EnvironmentLight.h
    Render/Frontend/EnvironmentLight.cpp
    Render/Frontend/OceanSurface.cpp
    Render/Frontend/OceanSurface.h
    Render/Frontend/RenderLight.h
    Render/Frontend/RenderLight.cpp
    Render/Frontend/RenderObject.h
    Render/Frontend/RenderObject.cpp
    Render/Frontend/RenderWorld.h
    Render/Frontend/RenderWorld.cpp
    Render/Frontend/RenderWorld_Draw.cpp
    Render/Frontend/RenderWorld_DebugTools.cpp
    Render/Frontend/RenderCamera.h
    Render/Frontend/RenderCamera.cpp
    Render/RenderUtils.h
    Render/RenderUtils.cpp
    Render/RenderInternal.h
    Render/Frontend/RenderPassDepthPSOResolver.h
    Render/Frontend/RenderPassDepthPSOResolver.cpp
    Render/Frontend/RenderPassPSOResolver.h
    Render/Frontend/RenderPassPSOResolver.cpp
    Render/Frontend/RenderPassBuilder.h
    Render/Frontend/RenderPassBuilder.cpp
    Render/Sampler.h
    Render/SamplerManager.h
    Render/SamplerManager.cpp
    Shaders/ShaderTypes.h
    Shaders/ShaderInterop.h)

file(GLOB_RECURSE SHADER_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.hlsli
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.shader)

file(GLOB_RECURSE MATERIAL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Materials/*.material)

set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${MATERIAL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

set(ALL_FILES
    ${SRC_FILES}
    ${SHADER_FILES}
    ${MATERIAL_FILES})

auto_source_group(${ALL_FILES})

include(FindDXC)
include(FindPIX)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}/Runtime/Public
    ${ENGINE_INCLUDE_DIR}/ThirdParty
    ${ENGINE_INCLUDE_DIR}/ThirdParty/DirectX12/include
    ${ENGINE_INCLUDE_DIR}/ThirdParty/DirectX12/include/d3dx12
    ${PIX_INCLUDE_DIR})

enable_precompiled_header(Precompiled.h Precompiled.cpp SRC_FILES)

add_executable(${PROJECT_NAME} WIN32 ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    #DEVELOPMENT_POSTFIX "_dev"
    OUTPUT_NAME ${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE YES)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

target_include_directories(${PROJECT_NAME} PUBLIC ${DXC_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} winmm.lib dxgi.lib dxguid.lib d3d12.lib d3dcompiler.lib)
target_link_libraries(${PROJECT_NAME} D3D12MemAlloc imgui_win32_dx12)
#target_link_libraries(${PROJECT_NAME} ${DXC_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${PIX_LIBRARY})
if (TARGET Tracy::TracyClient)
    target_link_libraries(${PROJECT_NAME} Tracy::TracyClient)
endif ()
target_link_libraries(${PROJECT_NAME} BlueshiftRuntime)
