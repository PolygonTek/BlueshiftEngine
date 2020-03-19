cmake_minimum_required(VERSION 2.8.12)

project(libpng)

set(SRC_FILES
    png.h
    pngconf.h
    pnglibconf.h
    pngdebug.h
    pnginfo.h
    pngpriv.h
    pngstruct.h
    png.c
    pngerror.c
    pngget.c
    pngmem.c
    pngpread.c
    pngread.c
    pngrio.c
    pngrtran.c
    pngrutil.c
    pngset.c
    pngtrans.c
    pngwio.c
    pngwrite.c
    pngwtran.c
    pngwutil.c
)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    list(APPEND SRC_FILES
        arm/arm_init.c
        arm/filter_neon_intrinsics.c
        arm/palette_neon_intrinsics.c
        #arm/filter_neon.S
    )
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^i?86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64*")
    list(APPEND SRC_FILES
        intel/intel_init.c
        intel/filter_sse2_intrinsics.c)
endif ()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} zlib)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
