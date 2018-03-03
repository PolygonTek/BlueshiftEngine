cmake_minimum_required(VERSION 2.8.12)

project(minizip)

set(SRC_FILES
  ioapi.h
  unzip.h
  zip.h
  "ioapi.c"   "unzip.c"   "zip.c"
)

add_definitions(-DUSE_FILE32API)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} zlib)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})