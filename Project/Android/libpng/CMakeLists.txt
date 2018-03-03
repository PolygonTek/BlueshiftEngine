Output Directory: ..\..\..\Build\Android\$(Platform)\$(Configuration)\
Intermidiate Directory: ..\..\..\Build\Android\$(ProjectName)\$(Platform)\$(Configuration)\
Min Android API Level : Android 5.0 (android-21)
Target Android API Level : Android 5.0 (android-21)
Additional Include Directory: ..\..\..\Source\Dependencies\freetype\include
Preprocesson Definition: 
Find what:[ \t]+([^ \t\r\n]+\.c\w*)
Replace with:    <ClCompile Include="..\..\..\Source\Dependencies\libpng\$1" />


cmake_minimum_required(VERSION 2.8.12)

project(libpng)

set(SRC_FILES
  png.h
  pngconf.h
  pnglibconf.h
  pngdebug.h
  pnginfo.h
  pngpriv.h
  pngstruct.h
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\png.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngerror.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngget.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngmem.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngpread.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngread.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngrio.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngrtran.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngrutil.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngset.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngtrans.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngwio.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngwrite.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngwtran.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\pngwutil.c" />
)

if (IOS_PLATFORM)
  list(APPEND SRC_FILES
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\arm\arm_init.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpng\arm\filter_neon_intrinsics.c" />
    #arm\filter_neon.S
  )
endif ()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES} ${ARM_FILES})

target_link_libraries(${PROJECT_NAME} zlib)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})