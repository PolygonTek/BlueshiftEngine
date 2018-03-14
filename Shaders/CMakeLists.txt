cmake_minimum_required(VERSION 2.8.12)

project(Shaders)

file(GLOB_RECURSE ALL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/**)

auto_source_group(${ALL_FILES})

add_custom_target(${PROJECT_NAME} SOURCES ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Blueshift)
