# This module defines:
# DXC_INCLUDE_DIR, where to find the headers
#
# DXC_LIBRARY
# DXC_FOUND
#

IF (WIN32)
  IF ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    SET(DXC_LIBDIR "x64")
  ELSE ()
    SET(DXC_LIBDIR "x86")
  ENDIF ()
ELSEIF (${ENGINE_ARCH} MATCHES "arm64")
  SET(DXC_LIBDIR "arm64")
ENDIF ()

SET(DXC_LIBNAME "dxcompiler")

FIND_PATH(DXC_INCLUDE_DIR "dxcapi.h"
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/dxc/inc")

FIND_LIBRARY(DXC_LIBRARY ${DXC_LIBNAME}
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/dxc"
    PATH_SUFFIXES "lib/${DXC_LIBDIR}")

#Once one of the calls succeeds the result variable will be set and stored in the cache so that no call will search again.

IF (DXC_LIBRARY AND DXC_INCLUDE_DIR)
    SET(DXC_FOUND "YES")
ELSE()
    SET(DXC_FOUND "NO")
ENDIF()

if (DXC_FOUND)
  message(STATUS "Found DXC: ${DXC_LIBRARY}")
else()
  message(STATUS "Not found DXC in ${DXC_SEARCH_PATHS}")
endif ()
