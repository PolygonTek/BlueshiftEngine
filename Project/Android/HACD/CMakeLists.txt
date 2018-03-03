Output Directory: ..\..\..\Build\Android\$(Platform)\$(Configuration)\
Intermidiate Directory: ..\..\..\Build\Android\$(ProjectName)\$(Platform)\$(Configuration)\
Min Android API Level : Android 5.0 (android-21)
Target Android API Level : Android 5.0 (android-21)
Additional Include Directory: ..\..\..\Source\Dependencies\HACD
Preprocesson Definition: 

cmake_minimum_required(VERSION 2.8.12)

project(HACD)

set(SRC_FILES
	hacdCircularList.h
	hacdGraph.h
	hacdHACD.h
	hacdICHull.h
	hacdManifoldMesh.h
	hacdVector.h
	hacdVersion.h
	hacdCircularList.inl
	hacdVector.inl
	hacdGraph.cpp
	hacdHACD.cpp
	hacdICHull.cpp
	hacdManifoldMesh.cpp
)

auto_source_group(${SRC_FILES})

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Dependencies)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\Library\${ENGINE_BUILD_PLATFORM_DIR})