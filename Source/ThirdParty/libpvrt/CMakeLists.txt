cmake_minimum_required(VERSION 2.8.12)

project(libpvrt)

set(SRC_FILES
    PVRTArray.h
    PVRTError.h
    PVRTGlobal.h
    PVRTMap.h
    PVRTMemoryFileSystem.h
    PVRTResourceFile.h
    PVRTString.h
    PVRTTexture.h
    PVRTError.cpp
    PVRTResourceFile.cpp
    PVRTString.cpp
    PVRTTexture.cpp
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
