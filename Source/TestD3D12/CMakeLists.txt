cmake_minimum_required(VERSION 3.5)

project(TestD3D12)

set(ALL_FILES
    Precompiled.h
    Precompiled.cpp
    D3D12App.h
    D3D12App.cpp
    D3D12Renderer.h
    D3D12Renderer.cpp
    D3D12DescriptorPool.h
    D3D12DescriptorPool.cpp
    D3D12SingleDescriptorAllocator.h
    D3D12SingleDescriptorAllocator.cpp
    D3D12Texture.h
    D3D12Texture.cpp
    D3D12VertexBuffer.h
    D3D12VertexBuffer.cpp
    D3D12IndexBuffer.h
    D3D12IndexBuffer.cpp
    D3D12CommandListPool.h
    D3D12CommandListPool.cpp
    D3D12FrameData.h
    D3D12FrameData.cpp
    WinMain.cpp
    WinResource.h
    WinRes/small.ico
    WinRes/TestD3D12.ico
    WinRes/TestD3D12.rc)

auto_source_group(${ALL_FILES})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}/Runtime/Public
    ${ENGINE_INCLUDE_DIR}/ThirdParty
    ${ENGINE_INCLUDE_DIR}/ThirdParty/DirectX12/include
    ${ENGINE_INCLUDE_DIR}/ThirdParty/DirectX12/include/d3dx12)

enable_precompiled_header(Precompiled.h Precompiled.cpp ALL_FILES)

add_executable(${PROJECT_NAME} WIN32 ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    #DEVELOPMENT_POSTFIX "_dev"
    OUTPUT_NAME ${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE YES)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

target_link_libraries(${PROJECT_NAME} winmm.lib dxgi.lib dxguid.lib d3d12.lib d3dcompiler.lib)
target_link_libraries(${PROJECT_NAME} BlueshiftRuntime)
