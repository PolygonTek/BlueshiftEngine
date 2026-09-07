cmake_minimum_required(VERSION 3.5)

project(meshoptimizer VERSION 1.2 LANGUAGES CXX)

set(SRC_FILES
    src/meshoptimizer.h
    src/allocator.cpp
    src/clusterizer.cpp
    src/indexanalyzer.cpp
    src/indexcodec.cpp
    src/indexgenerator.cpp
    src/meshletcodec.cpp
    src/meshletutils.cpp
    src/opacitymap.cpp
    src/overdrawoptimizer.cpp
    src/partition.cpp
    src/quantization.cpp
    src/rasterizer.cpp
    src/simplifier.cpp
    src/spatialorder.cpp
    src/stripifier.cpp
    src/tangentspace.cpp
    src/vcacheoptimizer.cpp
    src/vertexcodec.cpp
    src/vertexfilter.cpp
    src/vfetchoptimizer.cpp
)

auto_source_group(${SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER ThirdParty
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
