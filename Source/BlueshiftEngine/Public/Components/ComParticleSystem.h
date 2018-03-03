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

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    void                    UpdateSimulation(int currentTime);

    bool                    IsAlive() const;

    void                    Play();
    void                    Stop();
    void                    Resume();
    void                    Pause();

    int                     GetAliveParticleCount() const;

    void                    ResetParticles();

    Guid                    GetParticleSystemGuid() const;
    void                    SetParticleSystemGuid(const Guid &guid);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    virtual void            UpdateVisuals() override;
    void                    ChangeParticleSystem(const Guid &particleSystemGuid);
    void                    InitializeParticle(Particle *particle, const ParticleSystem::Stage *stage, float inCycleFraction) const;
    void                    ProcessTrail(Particle *particle, const ParticleSystem::Stage *stage, float genTimeDelta);
    void                    ComputeTrailPositionFromCustomPath(const ParticleSystem::CustomPathModule &customPathModule, const Particle *particle, float t, Particle::Trail *trail) const;
    void                    ParticleSystemReloaded();
    void                    TransformUpdated(const ComTransform *transform);

    bool                    playOnAwake;
    ParticleSystemAsset *   particleSystemAsset;
    bool                    simulationStarted;
    int                     currentTime;
    int                     stopTime;

    SceneEntity::Parms      sprite;
    int                     spriteHandle;
    Mesh *                  spriteReferenceMesh;
};

BE_NAMESPACE_END
