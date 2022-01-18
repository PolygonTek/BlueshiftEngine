cmake_minimum_required(VERSION 2.8.12)

project(imgui)

set(SRC_FILES
    imconfig.h
    imgui.h
    imgui.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
)

if (NOT PLATFORM_WINDOWS)
    set_source_files_properties(imgui_impl_win32.h imgui_impl_win32.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
endif ()

if (NOT PLATFORM_MACOS)
    set_source_files_properties(imgui_impl_osx.h imgui_impl_osx.mm PROPERTIES HEADER_FILE_ONLY TRUE)
endif ()

auto_source_group(${SRC_FILES})

add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
