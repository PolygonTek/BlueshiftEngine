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

include_directories(${ENGINE_INCLUDE_DIR}/ThirdParty)

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if (USE_LUAJIT)
    include(FindLuaJIT)
    set(LUA_LIBRARY ${LUAJIT_LIBRARY})

    target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}/ThirdParty/luaJIT/src>)
else ()
    set(LUA_LIBRARY lua)

    target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}/ThirdParty/lua>)
endif ()

target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})

