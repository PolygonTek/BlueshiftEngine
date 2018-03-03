cmake_minimum_required(VERSION 2.8.12)

project(libControl)

if (WIN32)
  include(FindOpenAL)
endif ()

set(ALL_FILES
  PreCompiled.h
  PreCompiled.cpp
  Protocol.h
  Client.h
  Client.cpp
  Control.cpp
)

auto_source_group(${ALL_FILES})

if (WIN32)
  include_directories(${OPENAL_INCLUDE_DIR})
endif ()

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${ENGINE_INCLUDE_DIR}/Engine/Public
  ${ENGINE_INCLUDE_DIR}/ThirdParty
  ${ENGINE_INCLUDE_DIR}/ThirdParty/asio/include
)

enable_precompiled_header(Precompiled.h Precompiled.cpp CORE_FILES)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Blueshift)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_RPATH 1)
#set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_NAME_DIR "@loader_path")

if (WIN32)
	  target_link_libraries(${PROJECT_NAME} winmm.lib imm32.lib)

	  find_package(OpenGL REQUIRED)
	  target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
elseif (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++0x")
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK "NO")
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99")
  #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS NO)
  #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug] YES)
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP YES)
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] NO)
  #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${CMAKE_CURRENT_LIST_DIR}/Precompiled.h)
  #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")

	    add_framework(${PROJECT_NAME} Foundation)
	    add_framework(${PROJECT_NAME} AppKit)
	    add_framework(${PROJECT_NAME} Carbon)
	    add_framework(${PROJECT_NAME} OpenGL)
	    add_framework(${PROJECT_NAME} CoreGraphics)
	    add_framework(${PROJECT_NAME} CoreVideo)
	    add_framework(${PROJECT_NAME} OpenAL)
	    add_framework(${PROJECT_NAME} AudioToolbox)
	    add_framework(${PROJECT_NAME} AVFoundation)
## OSX

endif ()

target_link_libraries(${PROJECT_NAME} 
	    zlib
	    minizip
	    libjpeg
	    libpng
	    libpvrt
	    rg_etc1
	    freetype
	    jsoncpp
	    Bullet
	    HACD
	    BlueshiftEngine
)
