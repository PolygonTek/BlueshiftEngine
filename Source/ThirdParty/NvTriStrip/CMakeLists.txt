cmake_minimum_required(VERSION 2.8.12)

project(NvTriStrip)

set(SRC_FILES
    include/NvTriStrip.h
    src/NvTriStrip/NvTriStrip.cpp
    src/NvTriStrip/NvTriStripObjects.h
    src/NvTriStrip/NvTriStripObjects.cpp
    src/NvTriStrip/VertexCache.h
    src/NvTriStrip/VertexCache.cpp
)

auto_source_group(${SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} 
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
