cmake_minimum_required(VERSION 2.8.12)

project(TestBase)

set(ALL_FILES
    Main.cpp
    TestContainer.h
    TestContainer.cpp
    TestMath.h
    TestMath.cpp
    TestSIMD.h
    TestSIMD.cpp
    TestCUDA.h
    TestCUDA.cpp
    TestLua.h
    TestLua.cpp)

auto_source_group(${ALL_FILES})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}/BlueshiftEngine/Public
    ${ENGINE_INCLUDE_DIR}/ThirdParty
)

if (ANDROID)
    # Export ANativeActivity_onCreate(),
    # Refer to: https://github.com/android-ndk/ndk/issues/381.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

    add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
else ()
    add_executable(${PROJECT_NAME} ${ALL_FILES})
endif ()

target_link_libraries(${PROJECT_NAME} BlueshiftEngine LuaCpp)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

if (USE_LUAJIT)
    if (APPLE AND NOT IOS)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pagezero_size 10000 -image_base 100000000")
    endif ()
endif ()

if (NOT IOS)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        PREFIX ""
        #DEBUG_POSTFIX "_d"
        OUTPUT_NAME ${PROJECT_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})
endif ()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} winmm.lib)
endif ()
