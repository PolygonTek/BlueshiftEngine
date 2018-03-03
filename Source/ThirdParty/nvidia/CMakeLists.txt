cmake_minimum_required(VERSION 2.8.12)

project(nvidia)

set(SRC_FILES
    nv_and_util/nv_native_app_glue.h
    nv_bitfont/nv_bitfont.h
    nv_egl_util/nv_egl_util.h
    nv_file/nv_apk_file.h
    nv_file/nv_file.h
    nv_glesutil/nv_draw_rect.h
    nv_glesutil/nv_images.h
    nv_hhdds/nv_hhdds.h
    nv_log/nv_log.h
    nv_math/NvVec.h
    nv_math/nv_math.h
    nv_math/nv_matrix.h
    nv_math/nv_quat.h
    nv_shader/nv_shader.h
    nv_thread/nv_thread.h
    nv_and_util/nv_native_app_glue.c
    nv_bitfont/nv_bitfont.cpp
    nv_egl_util/nv_egl_util.cpp
    nv_file/nv_apk_file.c
    nv_file/nv_file.c
    nv_glesutil/nv_draw_rect.cpp
    nv_glesutil/nv_images.cpp
    nv_hhdds/nv_hhdds.cpp
    nv_log/nv_log.cpp
    nv_math/nv_math.cpp
    nv_math/nv_matrix.cpp
    nv_math/nv_quat.cpp
    nv_shader/nv_shader.cpp
    nv_thread/nv_thread.c
)
include_directories(${PROJECT_SOURCE_DIR})


add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ThirdParty)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
