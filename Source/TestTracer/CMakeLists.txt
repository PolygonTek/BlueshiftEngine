cmake_minimum_required(VERSION 3.5)

project(TestTracer)

set(SRC_FILES
    App.h
    App.cpp
    Precompiled.h
    Precompiled.cpp
    WinMain.cpp
    Render/Camera.cpp
    Render/Camera.h
    Render/DIB.cpp
    Render/DIB.h
    Render/Options.h
    Render/PathTracer.cpp
    Render/PathTracer.h
    Render/Random.h
    Render/Renderer.cpp
    Render/Renderer.h
    Render/Sampler.cpp
    Render/Sampler.h
    Scene/Scene.cpp
    Scene/Scene.h
    Scene/ScenePresets.cpp
    Scene/ScenePresets.h
    Shading/BSDF.cpp
    Shading/BSDF.h
    Shading/BxDF.cpp
    Shading/BxDF.h
    Shading/DifferentialGeometry.h
    Shading/Fresnel.cpp
    Shading/Fresnel.h
    Shading/Glass.h
    Shading/Lambertian.cpp
    Shading/Lambertian.h
    Shading/Material.cpp
    Shading/Material.h
    Shading/Matte.h
    Shading/Metal.h
    Shading/Microfacet.cpp
    Shading/Microfacet.h
    Shading/Mirror.h
    Shading/ObjMat.h
    Shading/OrenNayar.cpp
    Shading/OrenNayar.h
    Shading/Phong.cpp
    Shading/Phong.h
    Shading/Plastic.h
    Shading/Specular.cpp
    Shading/Specular.h
    Shading/Texture.cpp
    Shading/Texture.h
    Geometry/BVH.cpp
    Geometry/BVH.h
    Geometry/Mesh.cpp
    Geometry/Mesh.h
    Geometry/Model.cpp
    Geometry/Model.h
    Geometry/ObjLoader.cpp
    Geometry/ObjLoader.h
    WinRes/TargetVer.h
    WinRes/WinResource.h
    WinRes/small.ico
    WinRes/void-tracer.ico
    WinRes/void-tracer.rc
)

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT_NAME} BlueshiftRuntime)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Test)

enable_precompiled_header(Precompiled.h Precompiled.cpp SRC_FILES)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /permissive-)
    target_link_libraries(${PROJECT_NAME} winmm comctl32)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/${ENGINE_BUILD_PLATFORM_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES})
