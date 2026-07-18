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
    GameObject.h
    GameObject.cpp
    GameLight.h
    GameLight.cpp
    Scene.h
    SceneRegistry.h
    SceneRegistry.cpp
    SceneTest.h
    SceneTest.cpp
    SceneDebugDraw.h
    SceneDebugDraw.cpp
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
    Render/RenderCvars.h
    Render/RenderCvars.cpp
    Render/ColorGradingLUTGenerator.h
    Render/ColorGradingLUTGenerator.cpp
    Render/ConvolutionBloomKernelGenerator.h
    Render/ConvolutionBloomKernelGenerator.cpp
    Render/GpuTiming.h
    Render/GpuTiming.cpp
    Render/RenderContext.h
    Render/RenderContext.cpp
    Render/RenderFrameData.h
    Render/RenderFrameData.cpp
    Render/GpuFFT.h
    Render/GpuFFT.cpp
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
    Render/RenderBackend.h
    Render/RenderBackend.cpp
    Render/RenderBackend_HiZCulling.cpp
    Render/RenderBackend_GenHiZ.cpp
    Render/RenderBackend_HiZDebug.cpp
    Render/RenderBackend_PostProcess.cpp
    Render/RenderBackend_DebugDraw.cpp
    Render/RenderBackend_DrawGenericSurf.cpp
    Render/RenderBackend_DrawSkySurf.cpp
    Render/RenderLight.h
    Render/RenderLight.cpp
    Render/RenderObject.h
    Render/RenderObject.cpp
    Render/RenderWorld.h
    Render/RenderWorld.cpp
    Render/RenderWorld_Draw.cpp
    Render/RenderWorld_DebugTools.cpp
    Render/RenderCamera.h
    Render/RenderCamera.cpp
    Render/RenderUtils.h
    Render/RenderUtils.cpp
    Render/RenderInternal.h
    Render/RenderPassDepthPSOResolver.h
    Render/RenderPassDepthPSOResolver.cpp
    Render/RenderPassPSOResolver.h
    Render/RenderPassPSOResolver.cpp
    Render/RenderPassBuilder.h
    Render/RenderPassBuilder.cpp
    Render/Sampler.h
    Render/Sampler.cpp
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
