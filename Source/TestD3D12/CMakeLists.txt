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
    Shaders/ShaderInterop.h
    Render/Anim.h
    Render/Anim.cpp
    Render/Anim_InternalData.cpp
    Render/AnimManager.h
    Render/AnimManager.cpp
    Render/BModel.h
    Render/TextureStreamingFeedback.h
    Render/TextureStreamingFeedback.cpp
    Render/VertexInputLayoutFactory.h
    Render/VertexInputLayoutFactory.cpp
    Render/RenderCvars.h
    Render/RenderCvars.cpp
    Render/ConvolutionBloomKernelGenerator.h
    Render/ConvolutionBloomKernelGenerator.cpp
    Render/RenderContext.h
    Render/RenderContext.cpp
    Render/RenderFrameData.h
    Render/RenderFrameData.cpp
    Render/GpuFFT.h
    Render/GpuFFT.cpp
    Render/ShaderMaterialTable.h
    Render/ShaderMaterialTable.cpp
    Render/ShaderSurfaceTable.h
    Render/ShaderSurfaceTable.cpp
    Render/RenderSystem.h
    Render/RenderSystem.cpp
    Render/RenderBackend.h
    Render/RenderBackend.cpp
    Render/RenderBackend_DrawGeneric.cpp
    Render/RenderBackend_DrawSky.cpp
    Render/RenderObject.h
    Render/RenderObject.cpp
    Render/RenderWorld.h
    Render/RenderWorld.cpp
    Render/RenderWorld_Draw.cpp
    Render/RenderWorld_DebugTools.cpp
    Render/RenderCamera.h
    Render/RenderCamera.cpp
    Render/RenderInternal.h
    Render/RenderPassBuilder.h
    Render/RenderPassBuilder.cpp
    Render/Material.h
    Render/Material.cpp
    Render/MaterialManager.h
    Render/MaterialManager.cpp
    Render/ShaderPipelineCache.h
    Render/ShaderPipelineCache.cpp
    Render/ShaderFeatureSet.h
    Render/ShaderFeatureSet.cpp
    Render/ShaderPermutationKey.h
    Render/ShaderPermutationKey.cpp
    Render/ShaderPipeline.h
    Render/ShaderPipeline.cpp
    Render/ShaderTemplate.h
    Render/ShaderTemplate.cpp
    Render/ShaderTemplateManager.h
    Render/ShaderTemplateManager.cpp
    Render/Mesh.h
    Render/Mesh.cpp
    Render/Mesh_InternalData.cpp
    Render/par_octasphere.h
    Render/Mesh_Create.cpp
    Render/MeshInstance.h
    Render/MeshInstance.cpp
    Render/SkinningJointCache.h
    Render/SkinningJointCache.cpp
    Render/MeshManager.h
    Render/MeshManager.cpp
    Render/SubMesh.h
    Render/SubMesh.cpp
    Render/GuiMesh.h
    Render/GuiMesh.cpp
    Render/Skeleton.h
    Render/Skeleton.cpp
    Render/SkeletonManager.h
    Render/SkeletonManager.cpp
    Render/Font.h
    Render/Font.cpp
    Render/FontFace.h
    Render/FontFaceBitmap.cpp
    Render/FontFaceTrueType.cpp
    Render/FontManager.h
    Render/FontManager.cpp
    Render/FontFile.h
    Render/TrueTypeFont.h
    Render/TrueTypeFont.cpp
    Render/Sampler.h
    Render/Sampler.cpp
    Render/Texture.h
    Render/Texture.cpp
    Render/TextureManager.h
    Render/TextureManager.cpp)

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
