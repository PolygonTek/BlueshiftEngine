cmake_minimum_required(VERSION 2.8.12)

project(lua)

set(SRC_FILES
  lapi.h
  lapi.c
  lauxlib.h
  lauxlib.c
  lbaselib.c
  lbitlib.c
  lcode.h
  lcode.c
  lcorolib.c
  lctype.h
  lctype.c
  ldblib.c
  ldebug.h
  ldebug.c
  ldo.h
  ldo.c
  ldump.c
  lfunc.c
  lfunc.h
  lgc.c
  lgc.h
  linit.c
  liolib.c
  llex.h
  llex.c
  llimits.h
  lmathlib.c
  lmem.c
  lmem.h
  loadlib.c
  lobject.c
  lobject.h
  lopcodes.c
  lopcodes.h
  loslib.c
  lparser.h
  lparser.c
  #lprefix.h
  lstate.h
  lstate.c
  lstring.h
  lstring.c
  lstrlib.c
  ltable.c
  ltable.h
  ltablib.c
  ltm.h
  ltm.c
  lua.h
  lua.hpp
  luaconf.h
  lualib.h
  lundump.h
  lundump.c
  #lutf8lib.c
  lvm.h
  lvm.c
  lzio.h
  lzio.c
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
