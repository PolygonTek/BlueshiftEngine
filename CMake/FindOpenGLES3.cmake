# Locate OpenGL ES 3
#
# This module defines:
# OPENGLES3_INCLUDE_DIR = where to find the headers
# OPENGLES3_LIBRARY = where to OpenGL ES3 library exist
# OPENGLES3_FOUND = YES or NO
#

if (WIN32)
    set(OPENGLES3_LIBDIR "Windows")
elseif (APPLE)
    set(OPENGLES3_LIBDIR "macOS")
elseif (LINUX)
    set(OPENGLES3_LIBDIR "Linux")
endif ()

if (APPLE)
    # do nothing
elseif (CMAKE_CL_64)
    set(OPENGLES3_LIBDIR ${OPENGLES3_LIBDIR}/x64)
elseif (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENGLES3_LIBDIR ${OPENGLES3_LIBDIR}/x64)
else ()
    set(OPENGLES3_LIBDIR ${OPENGLES3_LIBDIR}/x86)
endif ()

set(OPENGLES3_LIBNAME "libGLESv2")

find_path(OPENGLES3_INCLUDE_DIR "gl3.h"
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenGL/include/GLES3"
    NO_DEFAULT_PATH)

find_library(OPENGLES3_LIBRARY ${OPENGLES3_LIBNAME}
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenGL/lib/${OPENGLES3_LIBDIR}"
    NO_DEFAULT_PATH)

if (OPENGLES3_LIBRARY AND OPENGLES3_INCLUDE_DIR)
    set(OPENGLES3_FOUND "YES")
    message(STATUS "Found OpenGLES3: ${OPENGLES3_LIBRARY}")
else ()
    set(OPENGLES3_FOUND "NO")
    message(STATUS "Not found OpenGLES3 in ${OPENGLES3_SEARCH_PATHS}")
endif ()
