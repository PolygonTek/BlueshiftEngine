################################################################################
# Top-level CMakeLists.txt file for Blueshift Engine
################################################################################
cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

include(BlueshiftCommon)

option(BUILD_ENGINE "Build Blueshift Engine" OFF)
option(BUILD_PLAYER "Build Blueshift Player" OFF)
option(BUILD_EDITOR "Build Blueshift Editor" OFF)
option(BUILD_TEST "Build test projects" OFF)

if (BUILD_ENGINE)
    set(project_name BlueshiftEngine)

    set(ENGINE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source CACHE STRING "engine include directory")
    set(ENGINE_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Library CACHE STRING "engine library directory")
elseif (BUILD_PLAYER)
    set(project_name BlueshiftPlayer)
else ()
    message(FATAL_ERROR "Neither BUILD_ENGINE nor BUILD_PLAYER are set")
endif ()

project(${project_name})

set(VERSION_MAJOR 0 CACHE STRING "Project major version number.")
set(VERSION_MINOR 3 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

message(STATUS "Running from ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Version: ${CMAKE_VERSION}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION} ${CMAKE_SYSTEM_PROCESSOR}") 
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)

# Set this variable to specify a common place where CMake should put all executable files
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${CMAKE_GENERATOR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX "_d")

# Add DEBUG, _DEBUG definition to compiler in debug build
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")

# Settings for macOS, iOS
if (APPLE)
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")

    add_compile_options("-Wno-unused-parameter")
    add_compile_options("-Wno-unused-variable")
    add_compile_options("-Wno-unused-local-typedef")
    add_compile_options("-Wno-unused-function")
    add_compile_options("-Wno-reorder")
    add_compile_options("-Wno-inconsistent-missing-override")

    if (NOT IOS)
        add_definitions(-DIOS)

        set(CMAKE_OSX_SYSROOT "macosx")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9") # 10.9 Mavericks
        #set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
    endif ()
endif ()

# Settings for Android
if (ANDROID)
    add_definitions(-DANDROID)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1y")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif ()

# Settings for Windows
if (WIN32 AND MSVC)
    add_definitions(-D_UNICODE -DUNICODE)

    # Enable Function-Level Linking
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Gy")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy")
    
    # Force Synchronous PDB Writes
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
    
    # Remove RTTI
    string(REGEX REPLACE "/GR-?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    #set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    #set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")  
endif ()

################################################################################
if (XAMARIN)
    if (MSVC)
        set(ENGINE_BUILD_PLATFORM_DIR "XamarinWin64")
    elseif (APPLE)
        if (IOS)
            if (WITHDEBUG)
	            set(WITHDEBUG_POSTFIX "_d")
            endif ()
            set(ENGINE_BUILD_PLATFORM_DIR "XamarinIOS$(EFFECTIVE_PLATFORM_NAME)${WITHDEBUG_POSTFIX}")
        endif ()
    endif ()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__XAMARIN__")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__XAMARIN__")
    if (APPLE)
		if (WITHDEBUG)
			set(CMAKE_C_FLAGS_DEBUG "-g -D__XAMARIN__ -DBE_DEBUG -DDEBUG -D_DEBUG")
			set(CMAKE_C_FLAGS_MINSIZEREL "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
			set(CMAKE_C_FLAGS_RELEASE "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
			set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
			set(CMAKE_CXX_FLAGS_DEBUG "-g -D__XAMARIN__ -DBE_DEBUG -DDEBUG -D_DEBUG")
			set(CMAKE_CXX_FLAGS_MINSIZEREL "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
			set(CMAKE_CXX_FLAGS_RELEASE "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -D__XAMARIN__ -DBE_DEBUG -DNDEBUG")
		endif ()

		#set(CMAKE_OSX_ARCHITECTURES "armv7;arm64;i386")
		set(CMAKE_OSX_ARCHITECTURES "armv7;arm64")
    endif ()
else ()
    if (MSVC)
        set(ENGINE_BUILD_PLATFORM_DIR "Win64")
    elseif (APPLE)
        if (IOS)
            set(ENGINE_BUILD_PLATFORM_DIR "iOS$(EFFECTIVE_PLATFORM_NAME)")
        else ()
            set(ENGINE_BUILD_PLATFORM_DIR "macOS")
        endif ()
    elseif (ANDROID)
        set(ENGINE_BUILD_PLATFORM_DIR "Android")
    else ()
        set(ENGINE_BUILD_PLATFORM_DIR ${CMAKE_GENERATOR})
    endif ()
endif ()

################################################################################
# Print C/CXX FLAGS
################################################################################

set(FLAGS
    CMAKE_C_FLAGS
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
)

foreach(FLAG ${FLAGS})
    message(STATUS "${FLAG}: ${${FLAG}}")
endforeach()

################################################################################
# Sub projects
################################################################################
if (BUILD_ENGINE)
    add_subdirectory(Source/Dependencies/zlib)
    add_subdirectory(Source/Dependencies/minizip)
    add_subdirectory(Source/Dependencies/libjpeg)
    add_subdirectory(Source/Dependencies/libpng)
    add_subdirectory(Source/Dependencies/libpvrt)
    add_subdirectory(Source/Dependencies/etcpack)
    add_subdirectory(Source/Dependencies/etc2comp)
    add_subdirectory(Source/Dependencies/freetype)
    add_subdirectory(Source/Dependencies/libogg)
    add_subdirectory(Source/Dependencies/libvorbis)
    add_subdirectory(Source/Dependencies/jsoncpp)
    add_subdirectory(Source/Dependencies/tinyxml2)
    add_subdirectory(Source/Dependencies/NvTriStrip)
    add_subdirectory(Source/Dependencies/Bullet)
    add_subdirectory(Source/Dependencies/HACD)
  
    if (ANDROID)
        add_subdirectory(Source/Dependencies/nvidia)
    endif()
  
    if (USE_LUAJIT)
        add_subdirectory(Source/Dependencies/luaJIT)
    else ()
        add_subdirectory(Source/Dependencies/lua)
    endif ()
  
    add_subdirectory(Source/Dependencies/luasocket)
    add_subdirectory(Source/Dependencies/LuaCpp)
  
    add_subdirectory(Source/BlueshiftEngine)

    add_subdirectory(Shaders)
endif ()

if (BUILD_PLAYER)
    add_subdirectory(Source/BlueshiftPlayer)
endif ()

if (BUILD_EDITOR)
    add_subdirectory(Source/Dependencies/ToolWindowManager)  
    add_subdirectory(Source/BlueshiftEditor)
endif ()

if (BUILD_TEST)
    add_subdirectory(Source/TestBase)
    add_subdirectory(Source/TestRenderer)
endif ()
