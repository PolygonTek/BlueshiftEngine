cmake_minimum_required(VERSION 2.8.12)

project(LuaCpp)

set(ALL_FILES
  LuaCpp.h
  BaseFunc.h
  Class.h
  ClassFunc.h
  Ctor.h
  Dtor.h
  ExceptionHandler.h
  ExceptionTypes.h
  Func.h
  function.h
  LuaRef.h
  MetatableRegistry.h
  Obj.h
  ObjFunc.h
  Searcher.h
  Module.h
  LuaStack.h
  Reference.h
  Pointer.h
  Registry.h
  ResourceHandler.h
  Selector.h
  State.h
  Traits.h  
  Tuple.h
  Util.h
  BaseFunc.cpp
  MetatableRegistry.cpp
  Selector.cpp
  State.cpp
  Util.cpp
)

auto_source_group(${ALL_FILES})

include_directories(
  ${ENGINE_INCLUDE_DIR}/Dependencies
)

if (USE_LUAJIT)
  add_definitions(-DUSE_LUAJIT=1)
  include(FindLuaJIT)
  set(LUA_LIBRARY ${LUAJIT_LIBRARY})
else ()
  add_definitions(-DUSE_LUAJIT=0)  
  set(LUA_LIBRARY lua)
endif ()

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})

target_include_directories(${PROJECT_NAME} 
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARY})
