cmake_minimum_required(VERSION 3.5)

project(tinyexr)

set(ALL_FILES
    tinyexr.h
    exr_reader.hh
    streamreader.hh
)

auto_source_group(${ALL_FILES})

add_custom_target(${PROJECT_NAME} SOURCES ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER ThirdParty)
