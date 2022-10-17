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
#include "Scripting/LuaVM.h"
#include "Components/ComParticleSystem.h"

BE_NAMESPACE_BEGIN

struct StopMode {};

void LuaVM::RegisterParticleSystemComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComParticleSystem = module["ComParticleSystem"];

    _ComParticleSystem.SetClass<ComParticleSystem>(module["ComRenderable"]);
    _ComParticleSystem.AddClassMembers<ComParticleSystem>(
        "particle_system", &ComParticleSystem::GetParticleSystem,
        "set_particle_system", &ComParticleSystem::SetParticleSystem,
        "play_on_awake", &ComParticleSystem::playOnAwake,
        "is_alive", &ComParticleSystem::IsAlive,
        "play", &ComParticleSystem::Play,
        "stop", &ComParticleSystem::Stop,
        "resume", &ComParticleSystem::Resume,
        "pause", &ComParticleSystem::Pause);

    _ComParticleSystem["meta_object"] = ComParticleSystem::metaObject;

    LuaCpp::Selector _ComParticleSystem_StopMode = _ComParticleSystem["StopMode"];
    _ComParticleSystem_StopMode.SetClass<StopMode>();
    _ComParticleSystem_StopMode["StopEmitting"] = ComParticleSystem::StopMode::StopEmitting;
    _ComParticleSystem_StopMode["StopEmittingAndClear"] = ComParticleSystem::StopMode::StopEmittingAndClear;
}

BE_NAMESPACE_END
