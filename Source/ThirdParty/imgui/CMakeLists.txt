cmake_minimum_required(VERSION 3.5)

project(imgui)

set(IMGUI_CORE_FILES
    imconfig.h
    imgui.h
    imgui.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
)

auto_source_group(${IMGUI_CORE_FILES})

add_library(${PROJECT_NAME} STATIC ${IMGUI_CORE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})

if (PLATFORM_WINDOWS)
    set(IMGUI_WIN32_DX12_FILES
        backends/imgui_impl_win32.h
        backends/imgui_impl_win32.cpp
        backends/imgui_impl_dx12.h
        backends/imgui_impl_dx12.cpp
    )

    auto_source_group(${IMGUI_WIN32_DX12_FILES})

    add_library(imgui_win32_dx12 STATIC EXCLUDE_FROM_ALL ${IMGUI_WIN32_DX12_FILES})

    target_include_directories(imgui_win32_dx12 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backends)

    target_link_libraries(imgui_win32_dx12 PUBLIC
        imgui
        d3d12.lib
        dxgi.lib
        imm32.lib)

    set_target_properties(imgui_win32_dx12 PROPERTIES 
        FOLDER ThirdParty
        PREFIX ""
        #DEBUG_POSTFIX "_d"
        OUTPUT_NAME imgui_win32_dx12
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
endif ()
