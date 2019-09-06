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
#include "Render/ParticleSystem.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Particle System", ParticleSystemResource, Resource)
BEGIN_EVENTS(ParticleSystemResource)
END_EVENTS

void ParticleSystemResource::RegisterProperties() {
}

ParticleSystemResource::ParticleSystemResource() {
    particleSystem = nullptr;
}

ParticleSystemResource::~ParticleSystemResource() {
    if (particleSystem) {
        particleSystemManager.ReleaseParticleSystem(particleSystem);
    }
}

ParticleSystem *ParticleSystemResource::GetParticleSystem() {
    if (particleSystem) {
        return particleSystem;
    }
    const Str particleSystemPath = resourceGuidMapper.Get(asset->GetGuid());
    particleSystem = particleSystemManager.GetParticleSystem(particleSystemPath);
    return particleSystem;
}

void ParticleSystemResource::Rename(const Str &newName) {
    const Str particleSystemPath = resourceGuidMapper.Get(asset->GetGuid());
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(particleSystemPath);
    if (existingParticleSystem) {
        particleSystemManager.RenameParticleSystem(existingParticleSystem, newName);
    }
}

bool ParticleSystemResource::Reload() {
    const Str particleSystemPath = resourceGuidMapper.Get(asset->GetGuid());
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(particleSystemPath);
    if (existingParticleSystem) {
        existingParticleSystem->Reload();
        return true;
    }
    return false;
}

bool ParticleSystemResource::Save() {
    const Str particleSystemPath = resourceGuidMapper.Get(asset->GetGuid());
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(particleSystemPath);
    if (existingParticleSystem) {
        existingParticleSystem->Write(existingParticleSystem->GetHashName());
        return true;
    }
    return false;
}

BE_NAMESPACE_END
