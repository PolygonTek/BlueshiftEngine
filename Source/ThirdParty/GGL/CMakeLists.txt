cmake_minimum_required(VERSION 2.8.12)

project(GGL)

set(WINDOWS_SRC_FILES 
    gglcore32.h
    gglcore32.c
    gwgl.h
    gwgl.c)

set(LINUX_SRC_FILES
    gglcore32.h
    gglcore32.c
    gglx.h
    gglx.c)

set(ANDROID_SRC_FILES 
    ggles3.h
    ggles3.c
    gegl.h
    gegl.c)

set(IOS_SRC_FILES
    ggles3.h
    ggles3.c)

set(MACOS_SRC_FILES
    gglcore32.h
    gglcore32.c)

set(SRC_FILES ${WINDOWS_SRC_FILES} ${LINUX_SRC_FILES} ${ANDROID_SRC_FILES} ${IOS_SRC_FILES} ${MACOS_SRC_FILES})

set_source_files_properties(${SRC_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

if (PLATFORM_WINDOWS)
    set_source_files_properties(${WINDOWS_SRC_FILES} PROPERTIES HEADER_FILE_ONLY FALSE)
endif ()

if (PLATFORM_LINUX)
    set_source_files_properties(${LINUX_SRC_FILES} PROPERTIES HEADER_FILE_ONLY FALSE)
endif ()

if (PLATFORM_ANDROID)
    set_source_files_properties(${ANDROID_SRC_FILES} PROPERTIES HEADER_FILE_ONLY FALSE)
endif ()

if (PLATFORM_IOS)
    set_source_files_properties(${IOS_SRC_FILES} PROPERTIES HEADER_FILE_ONLY FALSE)
endif ()

if (PLATFORM_MACOS)
    set_source_files_properties(${MACOS_SRC_FILES} PROPERTIES HEADER_FILE_ONLY FALSE)
endif ()

auto_source_group(${SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (PlATFORM_WINDOWS AND USE_DESKTOP_EGL)
    target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}/ThirdParty/OpenGL/include>)
elseif (PLATFORM_IOS)
    target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}/ThirdParty/OpenGL/include>)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
