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
#include "Render/ParticleSystem.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterParticleSystem(LuaCpp::Module &module) {
    LuaCpp::Selector _ParticleSystem = module["ParticleSystem"];

    _ParticleSystem.SetClass<ParticleSystem>();
    _ParticleSystem.AddClassMembers<ParticleSystem>(
        "name", &ParticleSystem::GetName);

    _ParticleSystem["new"].SetFunc([]() {
        Guid newGuid = Guid::CreateGuid();
        Str particleSystemName = Str("ParticleSystem-") + newGuid.ToString();
        resourceGuidMapper.Set(newGuid, particleSystemName);
        return particleSystemManager.AllocParticleSystem(particleSystemName);
    });
    _ParticleSystem["release"].SetFunc([](ParticleSystem *particleSystem) {
        particleSystemManager.ReleaseParticleSystem(particleSystem);
    });
}

BE_NAMESPACE_END