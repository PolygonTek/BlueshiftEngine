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

    virtual void            Enable(bool enable) override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual void            Update() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    void                    UpdateSimulation(int currentTime);

    bool                    IsAlive() const;

    void                    Start();
    void                    Stop();
    void                    Resume();
    void                    Pause();

    int                     GetAliveParticleCount() const;

    void                    ResetParticles();

protected:
    virtual void            UpdateVisuals() override;
    void                    ChangeParticleSystem(const Guid &particleSystemGuid);
    void                    InitializeParticle(Particle *particle, const ParticleSystem::Stage *stage, float inCycleFraction) const;
    void                    ProcessTrail(Particle *particle, const ParticleSystem::Stage *stage, float genTimeDelta);
    void                    ComputeTrailPositionFromCustomPath(const ParticleSystem::CustomPathModule &customPathModule, const Particle *particle, float t, Particle::Trail *trail) const;
    void                    ParticleSystemReloaded();
    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);

    Guid                    GetParticleSystem() const;
    void                    SetParticleSystem(const Guid &guid);

    ParticleSystemAsset *   particleSystemAsset;
    bool                    simulationStarted;
    int                     currentTime;
    int                     stopTime;

    Mesh *                  spriteMesh;
    SceneEntity::Parms      sprite;
    int                     spriteHandle;
};

BE_NAMESPACE_END
