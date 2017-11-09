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
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("ParticleSystem", ParticleSystemAsset, Asset)
BEGIN_EVENTS(ParticleSystemAsset)
END_EVENTS

void ParticleSystemAsset::RegisterProperties() {
}

ParticleSystemAsset::ParticleSystemAsset() {
    particleSystem = nullptr;
}

ParticleSystemAsset::~ParticleSystemAsset() {
    if (!particleSystem) {
        const Str particleSystemPath = resourceGuidMapper.Get(GetGuid());
        particleSystem = particleSystemManager.FindParticleSystem(particleSystemPath);
    }

    if (particleSystem) {
        particleSystemManager.ReleaseParticleSystem(particleSystem, true);
    }
}

ParticleSystem *ParticleSystemAsset::GetParticleSystem() {
    if (particleSystem) {
        return particleSystem;
    }
    const Str particleSystemPath = resourceGuidMapper.Get(GetGuid());
    particleSystem = particleSystemManager.GetParticleSystem(particleSystemPath);
    return particleSystem;
}

void ParticleSystemAsset::Rename(const Str &newName) {
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(GetResourceFilename());
    if (existingParticleSystem) {
        particleSystemManager.RenameParticleSystem(existingParticleSystem, newName);
    }

    Asset::Rename(newName);
}

void ParticleSystemAsset::Reload() {
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(GetResourceFilename());
    if (existingParticleSystem) {
        existingParticleSystem->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

void ParticleSystemAsset::Save() {
    ParticleSystem *existingParticleSystem = particleSystemManager.FindParticleSystem(GetResourceFilename());
    if (existingParticleSystem) {
        existingParticleSystem->Write(existingParticleSystem->GetHashName());
        EmitSignal(&SIG_Modified, 0);
    }
}

BE_NAMESPACE_END
