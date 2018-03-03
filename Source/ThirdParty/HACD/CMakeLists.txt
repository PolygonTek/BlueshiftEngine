cmake_minimum_required(VERSION 2.8.12)

project(HACD)

set(SRC_FILES
	hacdCircularList.h
	hacdGraph.h
	hacdHACD.h
	hacdICHull.h
	hacdManifoldMesh.h
	hacdVector.h
	hacdVersion.h
	hacdCircularList.inl
	hacdVector.inl
	hacdGraph.cpp
	hacdHACD.cpp
	hacdICHull.cpp
	hacdManifoldMesh.cpp
)

auto_source_group(${SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
