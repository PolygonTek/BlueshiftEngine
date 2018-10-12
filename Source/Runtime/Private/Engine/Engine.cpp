// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "BlueshiftEngine.h"

BE_NAMESPACE_BEGIN

CmdArgs     Engine::args;
Str         Engine::baseDir;
Str         Engine::searchPath;

static streamOutFunc_t logFuncPtr = nullptr;
static streamOutFunc_t errFuncPtr = nullptr;

void Engine::InitBase(const char *baseDir, bool forceGenericSIMD, const streamOutFunc_t logFunc, const streamOutFunc_t errFunc) {
    setlocale(LC_ALL, "");

    logFuncPtr = logFunc;
    errFuncPtr = errFunc;

#if defined(__WIN32__) && defined(_DEBUG)
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

    // NOTE: can be replaced by setting '{,,ucrtbased}_crtBreakAlloc' (msvc2015) in debug watch window
    //_CrtSetBreakAlloc(123456);
#endif
    
    ByteOrder::Init();

    cmdSystem.Init();

    cvarSystem.Init();

    fileSystem.Init(baseDir);

    DetectCpu();

    SIMD::Init(forceGenericSIMD);

    PlatformTime::Init();

    Math::Init();
}

void Engine::ShutdownBase() {
    PlatformTime::Shutdown();
    
    SIMD::Shutdown();

    fileSystem.Shutdown();

    cvarSystem.Shutdown();

    cmdSystem.Shutdown();
}

static void RegisterEngineObjects() {
    Object::RegisterProperties();

    Asset::RegisterProperties();
    FolderAsset::RegisterProperties();
    TextureAsset::RegisterProperties();
    ShaderAsset::RegisterProperties();
    MaterialAsset::RegisterProperties();
    FontAsset::RegisterProperties();
    SkeletonAsset::RegisterProperties();
    MeshAsset::RegisterProperties();
    ParticleSystemAsset::RegisterProperties();
    AnimAsset::RegisterProperties();
    FbxAsset::RegisterProperties();
    JointMaskAsset::RegisterProperties();
    AnimControllerAsset::RegisterProperties();
    PrefabAsset::RegisterProperties();
    SoundAsset::RegisterProperties();
    MapAsset::RegisterProperties();
    ScriptAsset::RegisterProperties();

    Component::RegisterProperties();
    ComTransform::RegisterProperties();
    ComCollider::RegisterProperties();
    ComBoxCollider::RegisterProperties();
    ComSphereCollider::RegisterProperties();
    ComCapsuleCollider::RegisterProperties();
    ComConeCollider::RegisterProperties();
    ComCylinderCollider::RegisterProperties();
    ComMeshCollider::RegisterProperties();
    ComRigidBody::RegisterProperties();
    ComSensor::RegisterProperties();
    ComVehicleWheel::RegisterProperties();
    ComConstantForce::RegisterProperties();
    ComJoint::RegisterProperties();
    ComFixedJoint::RegisterProperties();
    ComHingeJoint::RegisterProperties();
    ComSocketJoint::RegisterProperties();
    ComSliderJoint::RegisterProperties();
    ComSpringJoint::RegisterProperties();
    ComWheelJoint::RegisterProperties();
    ComCharacterJoint::RegisterProperties();
    ComCharacterController::RegisterProperties();
    ComRenderable::RegisterProperties();
    ComMeshRenderer::RegisterProperties();
    ComStaticMeshRenderer::RegisterProperties();
    ComSkinnedMeshRenderer::RegisterProperties();
    ComAnimation::RegisterProperties();
    ComAnimator::RegisterProperties();
    ComParticleSystem::RegisterProperties();
    ComTextRenderer::RegisterProperties();
    ComLight::RegisterProperties();
    ComCamera::RegisterProperties();
    ComSpline::RegisterProperties();
    ComScript::RegisterProperties();
    ComAudioListener::RegisterProperties();
    ComAudioSource::RegisterProperties();

    Entity::RegisterProperties();
    Prefab::RegisterProperties();
    GameWorld::RegisterProperties();

    MapRenderSettings::RegisterProperties();
    TagLayerSettings::RegisterProperties();
    PhysicsSettings::RegisterProperties();
    PlayerSettings::RegisterProperties();
}

void Engine::Init(const InitParms *initParms) {
    Engine::baseDir = initParms->baseDir;
    Engine::searchPath = initParms->searchPath;
    Engine::args = initParms->args;

    Platform::Init();

    common.Init(Engine::baseDir);

    if (!Engine::searchPath.IsEmpty()) {
        fileSystem.SetSearchPath(Engine::searchPath);
    }

    RegisterEngineObjects();
}

void Engine::Shutdown() {
    common.Shutdown();

    Platform::Shutdown();
}

void Engine::RunFrame(int elapsedMsec) {
    common.RunFrame(elapsedMsec);
}

void Log(int logLevel, const wchar_t *fmt, ...) {
    wchar_t buffer[16384];
    va_list args;

    va_start(args, fmt);
    WStr::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    (*logFuncPtr)(logLevel, buffer);
}

void Error(int errLevel, const wchar_t *fmt, ...) {
    wchar_t buffer[16384];
    va_list args;

    va_start(args, fmt);
    WStr::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    (*errFuncPtr)(errLevel, buffer);
}

void Assert(bool expr) {
    if (!expr) {
        BE_ERRLOG(L"Assert Failed\n");
        assert(0);
    }
}

BE_NAMESPACE_END
