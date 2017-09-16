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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

ParticleSystem *        ParticleSystemManager::defaultParticleSystem;
ParticleSystemManager   particleSystemManager;

void ParticleSystemManager::Init() {
    particleSystemHashMap.Init(1024, 64, 64);

    // Create default particle system
    defaultParticleSystem = AllocParticleSystem("_defaultParticleSystem");
    defaultParticleSystem->permanence = true;
    defaultParticleSystem->CreateDefaultParticleSystem();
}

void ParticleSystemManager::Shutdown() {
    particleSystemHashMap.DeleteContents(true);
}

ParticleSystem *ParticleSystemManager::AllocParticleSystem(const char *hashName) {
    if (particleSystemHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs particleSystem already allocated", hashName);
    }

    ParticleSystem *particleSystem = new ParticleSystem;
    particleSystem->hashName = hashName;
    particleSystem->name = hashName;
    particleSystem->name.StripPath();
    particleSystem->name.StripFileExtension();
    particleSystem->refCount = 1;
    particleSystemHashMap.Set(particleSystem->hashName, particleSystem);

    return particleSystem;
}

void ParticleSystemManager::RenameParticleSystem(ParticleSystem *particleSystem, const Str &newName) {
    const auto *entry = particleSystemHashMap.Get(particleSystem->hashName);
    if (entry) {
        particleSystemHashMap.Remove(particleSystem->hashName);

        particleSystem->hashName = newName;
        particleSystem->name = newName;
        particleSystem->name.StripPath();
        particleSystem->name.StripFileExtension();

        particleSystemHashMap.Set(newName, particleSystem);
    }
}

void ParticleSystemManager::DestroyParticleSystem(ParticleSystem *particleSystem) {
    if (particleSystem->refCount > 1) {
        BE_WARNLOG(L"ParticleSystemManager::DestroyParticleSystem: particleSystem '%hs' has %i reference count\n", particleSystem->name.c_str(), particleSystem->refCount);
    }

    particleSystemHashMap.Remove(particleSystem->hashName);
    delete particleSystem;
}

void ParticleSystemManager::ReleaseParticleSystem(ParticleSystem *particleSystem, bool immediateDestroy) {
    if (particleSystem->permanence) {
        return;
    }

    if (particleSystem->refCount > 0) {
        particleSystem->refCount--;
    }

    if (immediateDestroy && particleSystem->refCount == 0) {
        DestroyParticleSystem(particleSystem);
    }
}

void ParticleSystemManager::DestroyUnusedParticleSystems() {
    Array<ParticleSystem *> removeArray;

    for (int i = 0; i < particleSystemHashMap.Count(); i++) {
        const auto *entry = particleSystemHashMap.GetByIndex(i);
        ParticleSystem *particleSystem = entry->second;

        if (particleSystem && !particleSystem->permanence && particleSystem->refCount == 0) {
            removeArray.Append(particleSystem);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyParticleSystem(removeArray[i]);
    }
}

ParticleSystem *ParticleSystemManager::FindParticleSystem(const char *hashName) const {
    const auto *entry = particleSystemHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

ParticleSystem *ParticleSystemManager::GetParticleSystem(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultParticleSystem;
    }

    ParticleSystem *particleSystem = FindParticleSystem(hashName);
    if (particleSystem) {
        particleSystem->refCount++;
        return particleSystem;
    }

    particleSystem = AllocParticleSystem(hashName);
    if (!particleSystem->Load(hashName)) {
        DestroyParticleSystem(particleSystem);
        return defaultParticleSystem;
    }

    return particleSystem;
}

void ParticleSystemManager::PrecacheParticleSystem(const char *name) {
    ParticleSystem *ps = GetParticleSystem(name);
    ReleaseParticleSystem(ps);
}

BE_NAMESPACE_END
