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
  TestLua.cpp
)

auto_source_group(${ALL_FILES})

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${ENGINE_INCLUDE_DIR}/BlueshiftEngine/Public
  ${ENGINE_INCLUDE_DIR}/Dependencies
)

add_executable(${PROJECT_NAME} ${ALL_FILES})

target_link_libraries(${PROJECT_NAME} 
  BlueshiftEngine tinyxml2 LuaCpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

if (NOT IOS)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
  set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})
endif ()

if (WIN32)
  target_link_libraries(${PROJECT_NAME} winmm.lib)
endif ()
