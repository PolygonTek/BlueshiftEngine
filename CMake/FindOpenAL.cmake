# This module defines:
# OPENAL_INCLUDE_DIR, where to find the headers
#
# OPENAL_LIBRARY
# OPENAL_FOUND
#

IF (WIN32)
  IF ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    SET(OPENAL_LIBDIR "Win64")
  ELSE ()
    SET(OPENAL_LIBDIR "Win32")
  ENDIF ()
ENDIF ()

SET(OPENAL_LIBNAME "OpenAL32")

FIND_PATH(OPENAL_INCLUDE_DIR "al.h"
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenAL/include")

FIND_LIBRARY(OPENAL_LIBRARY ${OPENAL_LIBNAME}
    PATHS "${CMAKE_SOURCE_DIR}/Source/ThirdParty/OpenAL"
    PATH_SUFFIXES "libs/${OPENAL_LIBDIR}")

#Once one of the calls succeeds the result variable will be set and stored in the cache so that no call will search again.

IF (OPENAL_LIBRARY AND OPENAL_INCLUDE_DIR)
    SET(OPENAL_FOUND "YES")
ELSE()
    SET(OPENAL_FOUND "NO")
ENDIF()

if (OPENAL_FOUND)
  message(STATUS "Found OPENAL: ${OPENAL_LIBRARY}")
else()
  message(STATUS "Not found OPENAL in ${OPENAL_SEARCH_PATHS}")
endif ()
