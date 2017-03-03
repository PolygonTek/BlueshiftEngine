Find what:[ \t]+([^ \t\r\n]+)
Replace with:    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\$1" />


cmake_minimum_required(VERSION 2.8.12)

project(libpvrt)

set(SRC_FILES
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTArray.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTError.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTGlobal.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTMap.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTMemoryFileSystem.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTResourceFile.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTString.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTTexture.h" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTError.cpp" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTResourceFile.cpp" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTString.cpp" />
    <ClCompile Include="..\..\..\Source\Dependencies\libpvrt\PVRTTexture.cpp" />
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})