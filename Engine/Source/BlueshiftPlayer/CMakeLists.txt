cmake_minimum_required(VERSION 2.8.12)

set(BS_APP_NAME "Blueshift Player" CACHE STRING "BS_APP_NAME variable defaulting")
set(BS_APP_IDENTIFIER "com.polygontek.BlueshiftPlayer" CACHE STRING "BS_APP_IDENTIFIER variable defaulting")
set(BS_APP_VERSION "1.0" CACHE STRING "BS_APP_VERSION variable defaulting")
set(BS_PRODUCT_NAME "BlueshiftPlayer" CACHE STRING "BS_PRODUCT_NAME variable defaulting")
set(BS_USE_ADMOB_REWARD_BASED_VIDEO FALSE CACHE BOOL "BS_USE_ADMOB_REWARD_BASED_VIDEO variable defaulting")

project(BlueshiftPlayer)

set(ALL_FILES
    Precompiled.h
    Precompiled.cpp
    Application.h
    Precompiled.h
    Application.cpp
)

if (XAMARIN AND IOS)
    list(APPEND ALL_FILES
        XamarinIOSMain.cpp
        Binding/XMCustomView.h 
        Binding/XMCustomView.m 
        Binding/XMCustomViewDelegate.h 
        Binding/XMUtilities.h
        Binding/XMUtilities.m
        Binding/XMBindingLibrarySample/XMBindingLibrarySample-Prefix.pch
        Binding/XMBindingLibrarySample/XMBindingLibrarySample.h
        Binding/XMBindingLibrarySample/XMBindingLibrarySample.m)
elseif (ANDROID)
    list(APPEND ALL_FILES
        AndroidMain.cpp)
elseif (WIN32)
    list(APPEND ALL_FILES
        WinMain.cpp
        WinResource.h
        Manifest.xml
        ResourceFiles/player.rc
        ResourceFiles/player.ico
        ResourceFiles/small.ico)
elseif (APPLE)
    if (IOS)
        list(APPEND ALL_FILES
            iOSDevice.h
            iOSDevice.mm
            iOSMain.mm
            iOSLaunchScreen.xib
            Assets.xcassets)

        if (BS_USE_ADMOB_REWARD_BASED_VIDEO)
            list(APPEND ALL_FILES 
                iOSAdMob.h
                iOSAdMob.mm)
        endif ()
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

if (ANDROID)
	include_directories(
		${ENGINE_INCLUDE_DIR}/Dependencies/nvidia)
endif ()

enable_precompiled_header(Precompiled.h Precompiled.cpp ALL_FILES)

if (XAMARIN)
    add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Blueshift)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})

    #APPLE
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++0x")
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK "NO")
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99")
    #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS NO)
    #set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug] YES)
    if (WITHDEBUG)
        set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP NO)
    else ()
        set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP YES)
        set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] NO)
    endif ()

    if (IOS)
		add_framework(${PROJECT_NAME} Foundation)
		add_framework(${PROJECT_NAME} UIKit)
		add_framework(${PROJECT_NAME} OpenGLES)
		add_framework(${PROJECT_NAME} CoreGraphics)
		add_framework(${PROJECT_NAME} QuartzCore)
		add_framework(${PROJECT_NAME} OpenAL)
		add_framework(${PROJECT_NAME} AudioToolbox)
		add_framework(${PROJECT_NAME} AVFoundation)
	endif ()
    target_link_libraries(${PROJECT_NAME} 
        zlib
        minizip
        libjpeg
        libpng
        libpvrt
        etcpack_lib
        Etc2CompLib
        libogg
        libvorbis
        freetype
        jsoncpp
        Bullet
        HACD
        lua
        LuaCpp
		luasocket
        BlueshiftEngine
    )
	if (ANDROID)
	    target_link_libraries(${PROJECT_NAME} nvidia)
	endif ()
else ()
    if (ANDROID)
	    add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
    else ()
	    add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${ALL_FILES})
    endif ()

    if (BUILD_ENGINE)
        set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Blueshift)
    endif ()

    if (NOT IOS)
        set_target_properties(${PROJECT_NAME} PROPERTIES 
            PREFIX ""
            DEBUG_POSTFIX "_d"
            OUTPUT_NAME ${PROJECT_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR})
    endif ()

    if (WIN32)
        find_package(OpenGL REQUIRED)
        target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} winmm.lib imm32.lib dxguid.lib dsound.lib ws2_32.lib)

        if (MSVC)
            set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE YES)

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "mt.exe" -manifest "${PROJECT_SOURCE_DIR}\\Manifest.xml" -outputresource:"$<TARGET_FILE:${PROJECT_NAME}>\;1"
                COMMENT "Adding manifest...")
        endif ()
    elseif (APPLE)
        # App name
        set(CMAKEVAR_APP_NAME ${BS_APP_NAME})
        # App version
        set(CMAKEVAR_BUNDLE_SHORT_VERSION_STRING ${BS_APP_VERSION})
        # Bundle version
        set(CMAKEVAR_BUNDLE_VERSION "1")

        set_target_properties(${PROJECT_NAME} PROPERTIES 
            XCODE_ATTRIBUTE_PRODUCT_NAME ${BS_PRODUCT_NAME}
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${BS_APP_IDENTIFIER}
            XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++0x"
            XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
            XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK "NO"
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
            XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99"
            #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS NO
            #XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Debug] YES
            XCODE_ATTRIBUTE_COPY_PHASE_STRIP YES
            XCODE_ATTRIBUTE_COPY_PHASE_STRIP[variant=Debug] NO
            XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${PROJECT_SOURCE_DIR}/Precompiled.h"
            XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
            XCODE_ATTRIBUTE_INSTALL_PATH "/Applications"
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO")

        if (IOS)
            set_target_properties(${PROJECT_NAME} PROPERTIES 
                XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 9.0
                XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # Universal (iPad + iPhone)
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
                #XCODE_ATTRIBUTE_DEVELOPMENT_TEAM XXX
                XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/iOSplist.in)

            set_source_files_properties(iOSLaunchScreen.xib Assets.xcassets PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

            add_framework(${PROJECT_NAME} Foundation)
            add_framework(${PROJECT_NAME} UIKit)
            add_framework(${PROJECT_NAME} OpenGLES)
            add_framework(${PROJECT_NAME} CoreGraphics)
            add_framework(${PROJECT_NAME} QuartzCore)
            add_framework(${PROJECT_NAME} OpenAL)
            add_framework(${PROJECT_NAME} AudioToolbox)
            add_framework(${PROJECT_NAME} AVFoundation)

            if (BS_USE_ADMOB_REWARD_BASED_VIDEO)
                add_definitions(-DUSE_ADMOB_REWARD_BASED_VIDEO=1)

                set(GoogleMobileAds ${ENGINE_INCLUDE_DIR}/Dependencies/GoogleMobileAds/GoogleMobileAds)

                if (${GoogleMobileAds} STREQUAL GoogleMobileAds-NOTFOUND)
                    message(ERROR ": Framework GoogleMobileAds not found")
                else ()
                    target_link_libraries(${PROJECT_NAME} ${GoogleMobileAds}.framework)
                    target_include_directories(${PROJECT_NAME} PUBLIC
                        $<BUILD_INTERFACE:${GoogleMobileAds}.framework/Headers>
                        $<INSTALL_INTERFACE:GoogleMobileAds>)
                endif ()

                add_framework(${PROJECT_NAME} AdSupport)
                add_framework(${PROJECT_NAME} CFNetwork)
                add_framework(${PROJECT_NAME} CoreMedia)
                add_framework(${PROJECT_NAME} CoreMotion)
                add_framework(${PROJECT_NAME} CoreTelephony)
                add_framework(${PROJECT_NAME} CoreVideo)
                add_framework(${PROJECT_NAME} GLKit)
                add_framework(${PROJECT_NAME} MessageUI)
                add_framework(${PROJECT_NAME} MediaPlayer)
                add_framework(${PROJECT_NAME} MobileCoreServices)
                add_framework(${PROJECT_NAME} Security)
                add_framework(${PROJECT_NAME} StoreKit)
                add_framework(${PROJECT_NAME} SystemConfiguration)
            endif ()
        else ()
            set(CMAKEVAR_COPYRIGHT "Copyright (c) 2014 PolygonTek. All rights reserved.")

            set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/macOSplist.in)

            set_source_files_properties(macOSMainMenu.xib PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            #source_group("${Resource}" FILES macOSMainMenu.xib)

            add_framework(${PROJECT_NAME} Foundation)
            add_framework(${PROJECT_NAME} AppKit)
            add_framework(${PROJECT_NAME} Carbon)
            add_framework(${PROJECT_NAME} OpenGL)
            add_framework(${PROJECT_NAME} CoreGraphics)
            add_framework(${PROJECT_NAME} CoreVideo)
            add_framework(${PROJECT_NAME} OpenAL)
            add_framework(${PROJECT_NAME} AudioToolbox)
            add_framework(${PROJECT_NAME} AVFoundation)
        endif ()
    endif ()

    if (BUILD_ENGINE)
        target_link_libraries(${PROJECT_NAME} BlueshiftEngine)

        if (IOS)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                #COMMAND rm -rf \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app/Data\"
                COMMAND mkdir -p \"$(PROJECT_DIR)/Source/Player/Data\"
                COMMAND mkdir -p \"$(PROJECT_DIR)/Source/Player/Config\"
                COMMAND mkdir -p \"$(PROJECT_DIR)/Source/Player/Scripts\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Source/Player/Data\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Source/Player/Config\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Source/Player/Scripts\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMENT "Copying data files"
                USES_TERMINAL)
        endif ()
    elseif (BUILD_PLAYER)
        set(LIB_FILES zlib minizip libjpeg libpng libpvrt etcpack_lib Etc2CompLib libogg libvorbis freetype jsoncpp Bullet HACD lua LuaCpp luasocket BlueshiftEngine)

        if (ANDROID)
            list(APPEND LIB_FILES nvidia android GLESv3 EGL OpenSLES)
        endif ()
    
        if (WIN32)
            foreach (FILE ${LIB_FILES})
                add_library(${FILE} STATIC IMPORTED)

                set_target_properties(${FILE} PROPERTIES
                    IMPORTED_LOCATION_DEBUG   ${ENGINE_LIBRARY_DIR}/${ENGINE_BUILD_PLATFORM_DIR}/Debug/${FILE}${CMAKE_DEBUG_POSTFIX}.lib
                    IMPORTED_LOCATION_RELEASE ${ENGINE_LIBRARY_DIR}/${ENGINE_BUILD_PLATFORM_DIR}/Release/${FILE}.lib)
            endforeach()
        elseif (APPLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                #COMMAND rm -rf \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app/Data\"
                COMMAND mkdir -p \"$(PROJECT_DIR)/Scripts\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Data\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Config\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMAND cp -Rf \"$(PROJECT_DIR)/Scripts\" \"$(TARGET_BUILD_DIR)/$(PRODUCT_NAME).app\"
                COMMENT "Copying data files"
                USES_TERMINAL)

            foreach (FILE ${LIB_FILES})
                add_library(${FILE} STATIC IMPORTED)

                set_target_properties(${FILE} PROPERTIES
                  IMPORTED_LOCATION_DEBUG   ${ENGINE_LIBRARY_DIR}/${ENGINE_BUILD_PLATFORM_DIR}/Debug/${FILE}${CMAKE_DEBUG_POSTFIX}.a
                  IMPORTED_LOCATION_RELEASE ${ENGINE_LIBRARY_DIR}/${ENGINE_BUILD_PLATFORM_DIR}/Release/${FILE}.a)
            endforeach ()
        endif ()

        target_link_libraries(${PROJECT_NAME} ${LIB_FILES})
    endif ()

    if (ANDROID)
	    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -landroid -lGLESv3 -lEGL -lOpenSLES")
	endif ()
endif ()

