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

void LuaVM::RegisterParticleSystemComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComTextRenderer = module["ComParticleSystem"];

    _ComTextRenderer.SetClass<ComParticleSystem>(module["ComRenderable"]);
    _ComTextRenderer.AddClassMembers<ComParticleSystem>(
        "is_alive", &ComParticleSystem::IsAlive,
        "play", &ComParticleSystem::Play,
        "stop", &ComParticleSystem::Stop,
        "resume", &ComParticleSystem::Resume,
        "pause", &ComParticleSystem::Pause);

    _ComTextRenderer["meta_object"] = ComParticleSystem::metaObject;
}

BE_NAMESPACE_END
