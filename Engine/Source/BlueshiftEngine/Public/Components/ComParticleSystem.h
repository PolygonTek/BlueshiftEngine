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

#pragma once

#include "ComRenderable.h"

BE_NAMESPACE_BEGIN

class ParticleSystemAsset;

class ComParticleSystem : public ComRenderable {
public:
    OBJECT_PROTOTYPE(ComParticleSystem);

    ComParticleSystem();
    virtual ~ComParticleSystem();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Update() override;

    void                    UpdateSimulation(int simulationTime);

    void                    StartSimulation();
    void                    PauseSimulation();
    void                    RestartSimulation();
    void                    StopSimulation();

    int                     GetAliveParticleCount() const;

protected:
    void                    ChangeParticleSystem(const Guid &particleSystemGuid);
    void                    ResetParticles();
    void                    InitializeParticle(Particle *particle, const ParticleSystem::Stage *stage, float inCycleFraction);
    void                    ProcessTrail(Particle *particle, const ParticleSystem::Stage *stage, float genTimeDelta);
    void                    ParticleSystemReloaded();
    void                    PropertyChanged(const char *classname, const char *propName);

    Guid                    GetParticleSystem() const;
    void                    SetParticleSystem(const Guid &guid);

    ParticleSystemAsset *   particleSystemAsset;
    Array<float>            startDelay;
    bool                    simulationStarted;
    int                     simulationTime;
};

BE_NAMESPACE_END
