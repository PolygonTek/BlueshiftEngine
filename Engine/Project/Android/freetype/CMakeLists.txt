Output Directory: ..\..\..\Build\Android\$(Platform)\$(Configuration)\
Intermidiate Directory: ..\..\..\Build\Android\$(ProjectName)\$(Platform)\$(Configuration)\
Min Android API Level : Android 5.0 (android-21)
Target Android API Level : Android 5.0 (android-21)
Additional Include Directory: ..\..\..\Source\Dependencies\freetype\include
Preprocesson Definition: FT2_BUILD_LIBRARY
Find what:[ \t]+([^ \t\r\n]+)
Replace with:    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\$1" />


cmake_minimum_required(VERSION 2.8.12)

project(freetype)

set(VERSION_MAJOR "2")
set(VERSION_MINOR "5")
set(VERSION_PATCH "5")
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# Compiler definitions for building the library
add_definitions(-DFT2_BUILD_LIBRARY)

# Specify library include directories
include_directories("${PROJECT_SOURCE_DIR}\include")

file(GLOB PUBLIC_HEADERS "include\*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "include\config\*.h")
file(GLOB PRIVATE_HEADERS "include\internal\*.h")

set(BASE_SRCS
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\autofit\autofit.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftadvanc.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftbbox.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftbdf.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftbitmap.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftcalc.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftcid.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftdbgmem.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftdebug.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftfstype.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftgasp.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftgloadr.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftglyph.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftgxval.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftinit.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftlcdfil.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftmm.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftobjs.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftotval.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftoutln.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftpatent.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftpfr.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftrfork.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftsnames.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftstream.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftstroke.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftsynth.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftsystem.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\fttrigon.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\fttype1.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftutil.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftwinfnt.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\base\ftxf86.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\bdf\bdf.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\bzip2\ftbzip2.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\cache\ftcache.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\cff\cff.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\cid\type1cid.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\gzip\ftgzip.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\lzw\ftlzw.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\pcf\pcf.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\pfr\pfr.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\psaux\psaux.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\pshinter\pshinter.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\psnames\psmodule.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\raster\raster.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\sfnt\sfnt.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\smooth\smooth.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\truetype\truetype.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\type1\type1.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\type42\type42.c" />
    <ClCompile Include="..\..\..\Source\Dependencies\freetype\src\winfonts\winfnt.c" />
)

set(ALL_FILES ${PUBLIC_HEADERS}
  ${PUBLIC_CONFIG_HEADERS}
  ${PRIVATE_HEADERS}
  ${BASE_SRCS}
)

source_group("Public Headers" FILES ${PUBLIC_HEADERS})
source_group("Public Config Headers" FILES ${PUBLIC_CONFIG_HEADERS})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})
auto_source_group(${BASE_SRCS})

include_directories("src\truetype")
include_directories("src\sfnt")
include_directories("src\autofit")
include_directories("src\smooth")
include_directories("src\raster")
include_directories("src\psaux")
include_directories("src\psnames")

add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} 
	PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}\include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})