# Locate LuaJIT
#
# This module defines:
# LUAJIT_INCLUDE_DIR = where to find the headers
# LUAJIT_LIBRARY = where to library exist
# LUAJIT_FOUND = YES or NO
#

if (APPLE)
    set(LUAJIT_LIBDIR "clang")
elseif (CMAKE_COMPILER_IS_GNUCXX)
    set(LUAJIT_LIBDIR "gcc4")
elseif (MSVC15 OR NOT (MSVC_VERSION LESS 1910))
    set(LUAJIT_LIBDIR "vs2017")
elseif (MSVC14 OR NOT (MSVC_VERSION LESS 1900))
    set(LUAJIT_LIBDIR "vs2015")
endif ()

if (APPLE)
    # do nothing
elseif (CMAKE_CL_64)
    set(LUAJIT_LIBDIR ${LUAJIT_LIBDIR}/x64)
elseif (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LUAJIT_LIBDIR ${LUAJIT_LIBDIR}/x64)
else ()
    set(LUAJIT_LIBDIR ${LUAJIT_LIBDIR}/x86)
endif ()

if (APPLE)
    set(LUAJIT_LIBNAME "libluajit.a")
elseif (CMAKE_COMPILER_IS_GNUCXX)
    set(LUAJIT_LIBNAME "lua51")
else ()
    set(LUAJIT_LIBNAME "lua51")
endif ()

find_path(LUAJIT_INCLUDE_DIR "lua.h"
    PATHS "${ENGINE_INCLUDE_DIR}/ThirdParty/LuaJIT"
    PATH_SUFFIXES "src")

find_library(LUAJIT_LIBRARY ${LUAJIT_LIBNAME}
    PATHS "${ENGINE_INCLUDE_DIR}/ThirdParty/LuaJIT"
    PATH_SUFFIXES "lib/${LUAJIT_LIBDIR}")

if (LUAJIT_LIBRARY AND LUAJIT_INCLUDE_DIR)
    set(LUAJIT_FOUND "YES")
    message(STATUS "Found LuaJIT: ${LUAJIT_LIBRARY}")
else ()
    set(LUAJIT_FOUND "NO")
    message(STATUS "Not found LuaJIT in ${LUAJIT_SEARCH_PATHS}")
endif ()
