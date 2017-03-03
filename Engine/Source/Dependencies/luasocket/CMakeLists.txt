cmake_minimum_required(VERSION 2.8.12)

project(luasocket)

set(SRC_FILES
  auxiliar.c
  auxiliar.h
  buffer.c
  buffer.h
  except.c
  except.h
  inet.c
  inet.h
  io.c
  io.h
  luasocket.c
  luasocket.h
  options.c
  options.h
  select.c
  select.h
  tcp.c
  tcp.h
  timeout.c
  timeout.h
  udp.c
  udp.h
)

if (WIN32 AND NOT ANDROID)
  list(APPEND SRC_FILES
    wsocket.c
    wsocket.h)
else ()
  list(APPEND SRC_FILES
    usocket.c
    usocket.h)
endif ()

include_directories(
  ${ENGINE_INCLUDE_DIR}/Dependencies/lua
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
