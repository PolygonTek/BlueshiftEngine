cmake_minimum_required(VERSION 2.8.12)

if (DEFINED ENV{QT_DIR})
  set(ENV{PATH} "$ENV{PATH};$ENV{QT_DIR}/bin")
  set(CMAKE_PREFIX_PATH "$ENV{QT_DIR} ${CMAKE_PREFIX_PATH}")
endif()

project(ToolWindowManager)

set(SOURCES 
    ToolWindowManager.cpp
    ToolWindowManagerArea.cpp
    ToolWindowManagerSplitter.cpp
    ToolWindowManagerTabBar.cpp
    ToolWindowManagerWrapper.cpp
)

set(HEADERS
    ToolWindowManager.h
    ToolWindowManagerArea.h
    ToolWindowManagerSplitter.h
    ToolWindowManagerTabBar.h
    ToolWindowManagerWrapper.h
)

set(MOC_SOURCES ${HEADERS})

find_package(Qt5Core)
find_package(Qt5Widgets)

qt5_wrap_cpp(OUT_MOC_FILES ${MOC_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} ${OUT_MOC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)

target_include_directories(${PROJECT_NAME} 
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
