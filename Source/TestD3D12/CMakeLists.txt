cmake_minimum_required(VERSION 3.5)

project(TestD3D12)

set(SRC_FILES
    Precompiled.h
    Precompiled.cpp
    RHI.h
    RHI.cpp
    D3D12Renderer/D3D12Common.h
    D3D12Renderer/D3D12Renderer.h
    D3D12Renderer/D3D12Renderer.cpp
    D3D12Renderer/D3D12SwapChain.h
    D3D12Renderer/D3D12SwapChain.cpp
    D3D12Renderer/D3D12RootDescriptorPool.h
    D3D12Renderer/D3D12RootDescriptorPool.cpp
    D3D12Renderer/D3D12DescriptorPool.h
    D3D12Renderer/D3D12DescriptorPool.cpp
    D3D12Renderer/D3D12Buffer.h
    D3D12Renderer/D3D12Buffer.cpp
    D3D12Renderer/D3D12Shader.h
    D3D12Renderer/D3D12Shader.cpp
    D3D12Renderer/D3D12Texture.h
    D3D12Renderer/D3D12Texture.cpp
    D3D12Renderer/D3D12Sampler.h
    D3D12Renderer/D3D12Sampler.cpp
    D3D12Renderer/D3D12VertexBuffer.h
    D3D12Renderer/D3D12VertexBuffer.cpp
    D3D12Renderer/D3D12IndexBuffer.h
    D3D12Renderer/D3D12IndexBuffer.cpp
    D3D12Renderer/D3D12ConstantBuffer.h
    D3D12Renderer/D3D12ConstantBuffer.cpp
    D3D12Renderer/D3D12PipelineState.h
    D3D12Renderer/D3D12PipelineState.cpp
    D3D12Renderer/D3D12Query.h
    D3D12Renderer/D3D12Query.cpp
    D3D12Renderer/D3D12CommandList.h
    D3D12Renderer/D3D12CommandList.cpp
    D3D12Renderer/D3D12CommandListPool.h
    D3D12Renderer/D3D12CommandListPool.cpp
    D3D12Renderer/D3D12FrameData.h
    D3D12Renderer/D3D12FrameData.cpp
    Shaders/ShaderInterop.h
    App.h
    App.cpp
    RenderContext.h
    RenderContext.cpp
    RenderFrameData.h
    RenderFrameData.cpp
    RenderSystem.h
    RenderSystem.cpp
    RenderBackEnd.h
    RenderBackEnd.cpp
    RenderObject.h
    RenderObject.cpp
    RenderWorld.h
    RenderWorld.cpp
    RenderCamera.h
    RenderCamera.cpp
    RenderInternal.h
    VisObject.cpp
    Mesh.h
    Mesh.cpp
    CubeMesh.h
    CubeMesh.cpp
    Sampler.h
    Sampler.cpp
    GameObject.h
    GameObject.cpp
    WinMain.cpp
    WinResource.h
    WinRes/small.ico
    WinRes/TestD3D12.ico
    WinRes/TestD3D12.rc)

file(GLOB_RECURSE SHADER_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/**.hlsl)

set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

set(ALL_FILES
    ${SRC_FILES}
    ${SHADER_FILES})

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
target_link_libraries(${PROJECT_NAME} D3D12MemAlloc)
#target_link_libraries(${PROJECT_NAME} ${DXC_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${PIX_LIBRARY})
target_link_libraries(${PROJECT_NAME} BlueshiftRuntime)
