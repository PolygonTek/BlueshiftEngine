cmake_minimum_required(VERSION 2.8.12)

project(etcpack_lib)

set(SRC_FILES
  etcpack_lib.h
  etcdec.cxx
  etcpack.cxx
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
