# Locate PIX
#
# This module defines:
# PIX_INCLUDE_DIR = where to find the headers
# PIX_LIBRARY = where to library exist
# PIX_FOUND = YES or NO
#

find_path(PIX_INCLUDE_DIR "pix3.h"
    PATHS "${ENGINE_INCLUDE_DIR}/ThirdParty/PIX"
    PATH_SUFFIXES "include")

find_library(PIX_LIBRARY "WinPixEventRuntime"
    PATHS "${ENGINE_INCLUDE_DIR}/ThirdParty/PIX"
    PATH_SUFFIXES "lib")

if (PIX_INCLUDE_DIR AND PIX_LIBRARY)
    set(PIX_FOUND "YES")
    message(STATUS "Found PIX: ${PIX_LIBRARY}")
else ()
    set(PIX_FOUND "NO")
    message(STATUS "Not found PIX in ${PIX_SEARCH_PATHS}")
endif ()
