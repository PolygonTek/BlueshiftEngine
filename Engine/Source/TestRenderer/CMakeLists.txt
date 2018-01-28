cmake_minimum_required(VERSION 2.8.12)

project(TestRenderer)

set(ALL_FILES
    Precompiled.h
    Precompiled.cpp

    Application.h
    Application.cpp
)

if (WIN32)
    list(APPEND ALL_FILES
        WinMain.cpp
        WinResource.h
        ResourceFiles/small.ico
        ResourceFiles/TestRenderer.ico
        ResourceFiles/TestRenderer.rc)
elseif (APPLE)
    if (IOS)
        list(APPEND ALL_FILES
            iOSMain.mm
            iOSLaunchScreen.xib)
    else ()
        list(APPEND ALL_FILES
            macOSMain.mm
            macOSMainMenu.xib)
    endif ()
endif ()

auto_source_group(${ALL_FILES})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}/BlueshiftEngine/Public
    ${ENGINE_INCLUDE_DIR}/Dependencies
)

enable_precompiled_header(Precompiled.h Precompiled.cpp ALL_FILES)

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${ALL_FILES})

target_link_libraries(${PROJECT_NAME} BlueshiftEngine)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

if (NOT IOS)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        PREFIX ""
        DEBUG_POSTFIX "_d"
        OUTPUT_NAME ${PROJECT_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})
endif ()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} winmm.lib)

    if (MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE YES)
    endif ()
elseif (APPLE)
    # App name
    set(CMAKEVAR_APP_NAME "Test Renderer")
    # App version
    set(CMAKEVAR_BUNDLE_SHORT_VERSION_STRING "1.0")
    # Bundle version
    set(CMAKEVAR_BUNDLE_VERSION "1")

    set_target_properties(${PROJECT_NAME} PROPERTIES 
        XCODE_ATTRIBUTE_PRODUCT_NAME "TestRenderer"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.polygontek.TestRenderer"
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++0x"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
        XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK "NO"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
        XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99"
        #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS NO
        #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug] YES
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP YES
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] NO
        #XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${CMAKE_CURRENT_LIST_DIR}/Precompiled.h
        #XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
        XCODE_ATTRIBUTE_INSTALL_PATH "/Applications"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO")

    if (IOS)
        set_target_properties(${PROJECT_NAME} PROPERTIES 
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET}
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # Universal (iPad + iPhone)
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/iOSplist.in)

        add_framework(${PROJECT_NAME} Foundation)
        add_framework(${PROJECT_NAME} UIKit)
    else ()
        set(CMAKEVAR_COPYRIGHT "Copyright (c) 2014 PolygonTek. All rights reserved.")

        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/macOSplist.in)

        set_source_files_properties(macOSMainMenu.xib PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        add_framework(${PROJECT_NAME} Foundation)
        add_framework(${PROJECT_NAME} AppKit)
    endif ()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND rm -rf "\"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app/Data\""
        COMMAND cp -Rf "\"$(PROJECT_DIR)/Data\"" "\"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app/Data\""
        COMMENT Copying data files)
endif ()
