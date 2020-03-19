# Locate EGL
#
# This module defines:
# EGL_INCLUDE_DIR = where to find the headers
# EGL_LIBRARY = where to EGL library exist
# EGL_FOUND = YES or NO
#

if (WIN32)
    set(EGL_LIBDIR "Windows")
elseif (APPLE)
    set(EGL_LIBDIR "macOS")
elseif (LINUX)
    set(EGL_LIBDIR "Linux")
endif ()

if (APPLE)
    # do nothing
elseif (CMAKE_CL_64)
    set(EGL_LIBDIR ${EGL_LIBDIR}/x64)
elseif (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EGL_LIBDIR ${EGL_LIBDIR}/x64)
else ()
    set(EGL_LIBDIR ${EGL_LIBDIR}/x86)
endif ()

set(EGL_LIBNAME "libEGL")

find_path(EGL_INCLUDE_DIR "egl.h"
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenGL/include/EGL"
    NO_DEFAULT_PATH)

find_library(EGL_LIBRARY ${EGL_LIBNAME}
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenGL/lib/${EGL_LIBDIR}"
    NO_DEFAULT_PATH)

if (EGL_LIBRARY AND EGL_INCLUDE_DIR)
    set(EGL_FOUND "YES")
    message(STATUS "Found EGL: ${EGL_LIBRARY}")
else ()
    set(EGL_FOUND "NO")
    message(STATUS "Not found EGL in ${EGL_SEARCH_PATHS}")
endif ()
