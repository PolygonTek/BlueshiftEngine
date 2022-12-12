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
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "Components/Transform/ComTransform.h"
#include "Components/Renderable/ComParticleSystem.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Particle System", ComParticleSystem, ComRenderable)
BEGIN_EVENTS(ComParticleSystem)
END_EVENTS

void ComParticleSystem::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("particleSystem", "Particle System", Guid, GetParticleSystemGuid, SetParticleSystemGuid, GuidMapper::defaultParticleSystemGuid, 
        "", PropertyInfo::Flag::Editor).SetMetaObject(&ParticleSystemResource::metaObject);
    REGISTER_PROPERTY("playOnAwake", "Play On Awake", bool, playOnAwake, true, 
        "", PropertyInfo::Flag::Editor);
}

ComParticleSystem::ComParticleSystem() {
    updatable = true;
}

ComParticleSystem::~ComParticleSystem() {
    Purge(false);
}

void ComParticleSystem::Purge(bool chainPurge) {
#if WITH_EDITOR
    if (spriteDef.mesh) {
        meshManager.ReleaseMesh(spriteDef.mesh);
        spriteDef.mesh = nullptr;
    }

    if (spriteReferenceMesh) {
        meshManager.ReleaseMesh(spriteReferenceMesh);
        spriteReferenceMesh = nullptr;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif

    if (renderObjectDef.particleSystem) {
        particleSystemManager.ReleaseParticleSystem(renderObjectDef.particleSystem);
        renderObjectDef.particleSystem = nullptr;
    }

    if (renderObjectDef.stageParticles.Count() > 0) {
        for (int stageIndex = 0; stageIndex < renderObjectDef.stageParticles.Count(); stageIndex++) {
            Mem_Free(renderObjectDef.stageParticles[stageIndex]);
        }

        renderObjectDef.stageParticles.Clear();
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComParticleSystem::Init() {
    ComRenderable::Init();

    currentTime = 0;
    stopEmitTime = 0;

    simulationStarted = false;

    ComTransform *transform = GetEntity()->GetTransform();

#if WITH_EDITOR
    // 3d spriteDef
    spriteReferenceMesh = meshManager.GetMesh("_defaultQuadMesh");

    spriteDef.flags = RenderObject::Flag::Billboard;
    spriteDef.layer = TagLayerSettings::BuiltInLayer::Editor;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTextureWithoutTextureInfo("Data/EditorUI/ParticleSystem.png", Texture::Flag::Clamp | Texture::Flag::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::TextureHint::Sprite);
    textureManager.ReleaseTexture(spriteTexture);

    spriteDef.mesh = spriteReferenceMesh->InstantiateMesh(Mesh::Type::Static);
    spriteDef.aabb = spriteReferenceMesh->GetAABB();
    spriteDef.worldMatrix = transform->GetMatrixNoScale();
    spriteDef.materialParms[RenderObject::MaterialParm::Red] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Green] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Blue] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Alpha] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::TimeOffset] = 0.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::TimeScale] = 1.0f;
#endif

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComParticleSystem::TransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized.
    SetInitialized(true);

    UpdateVisuals();
}

void ComParticleSystem::ChangeParticleSystem(const Guid &particleSystemGuid) {
#if WITH_EDITOR
    // Disconnect with previously connected particleSystem asset.
    if (particleSystemAsset) {
        particleSystemAsset->Disconnect(&Asset::SIG_Reloaded, this);
        particleSystemAsset = nullptr;
    }
#endif

    // Release the previously used particleSystem.
    if (renderObjectDef.particleSystem) {
        particleSystemManager.ReleaseParticleSystem(renderObjectDef.particleSystem);
        renderObjectDef.particleSystem = nullptr;
    }

    // Get the new particleSystem.
    const Str particleSystemPath = resourceGuidMapper.Get(particleSystemGuid);
    renderObjectDef.particleSystem = particleSystemManager.GetParticleSystem(particleSystemPath);

    ResetParticles();

#if WITH_EDITOR
    // Need to particleSystem asset to be reloaded in editor.
    particleSystemAsset = (Asset *)Asset::FindInstance(particleSystemGuid);
    if (particleSystemAsset) {
        particleSystemAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComParticleSystem::ParticleSystemReloaded, SignalObject::ConnectionType::Queued);
    }
#endif
}

void ComParticleSystem::ResetParticles() {
    renderObjectDef.stageStartDelay.SetCount(renderObjectDef.particleSystem->NumStages());

    // Free memory used for particles.
    if (renderObjectDef.stageParticles.Count() > 0) {
        for (int stageIndex = 0; stageIndex < renderObjectDef.stageParticles.Count(); stageIndex++) {
            Mem_Free(renderObjectDef.stageParticles[stageIndex]);
        }

        renderObjectDef.stageParticles.Clear();
    }

    renderObjectDef.stageParticles.SetCount(renderObjectDef.particleSystem->NumStages());

    for (int stageIndex = 0; stageIndex < renderObjectDef.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = renderObjectDef.particleSystem->GetStage(stageIndex);

        renderObjectDef.stageStartDelay[stageIndex] = stage->standardModule.startDelay.Evaluate(RANDOM_FLOAT(0, 1), 0);

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) ? stage->trailsModule.count : 0;
        int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;
        int size = stage->standardModule.count * particleSize;

        renderObjectDef.stageParticles[stageIndex] = (Particle *)Mem_Alloc(size);
        memset(renderObjectDef.stageParticles[stageIndex], 0, size);
    }
}

void ComParticleSystem::ClearParticles() {
    for (int stageIndex = 0; stageIndex < renderObjectDef.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = renderObjectDef.particleSystem->GetStage(stageIndex);

        renderObjectDef.stageStartDelay[stageIndex] = stage->standardModule.startDelay.Evaluate(RANDOM_FLOAT(0, 1), 0);

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) ? stage->trailsModule.count : 0;
        int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;
        int size = stage->standardModule.count * particleSize;

        memset(renderObjectDef.stageParticles[stageIndex], 0, size);
    }
}

void ComParticleSystem::Awake() {
    if (playOnAwake) {
        Play();
    }
}

void ComParticleSystem::OnActive() {
    ResetParticles();

    ComRenderable::OnActive();
}

void ComParticleSystem::OnInactive() {
#if WITH_EDITOR
    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif

    ComRenderable::OnInactive();
}

bool ComParticleSystem::HasRenderObject(int renderObjectHandle) const {
#if WITH_EDITOR
    if (this->spriteHandle == renderObjectHandle) {
        return true;
    }
#endif

    return ComRenderable::HasRenderObject(renderObjectHandle);
}

int ComParticleSystem::GetAliveParticleCount() const {
    int aliveCount = 0;

    for (int stageIndex = 0; stageIndex < renderObjectDef.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = renderObjectDef.particleSystem->GetStage(stageIndex);
        const ParticleSystem::StandardModule &standardModule = stage->standardModule;

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) ? stage->trailsModule.count : 0;

        for (int particleIndex = 0; particleIndex < standardModule.count; particleIndex++) {
            int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;

            // Get the particle pointer with the given particle index.
            Particle *particle = (Particle *)((byte *)renderObjectDef.stageParticles[stageIndex] + particleIndex * particleSize);

            if (particle->alive) {
                aliveCount++;
            }
        }
    }

    return aliveCount;
}

void ComParticleSystem::Update() {
    int elapsedTime = GetGameWorld()->GetTime() - GetGameWorld()->GetPrevTime();

    currentTime += elapsedTime;

    UpdateSimulation(currentTime);
}

void ComParticleSystem::UpdateSimulation(int currentTime) {
    if (!simulationStarted) {
        return;
    }

    renderObjectDef.time = currentTime;
    renderObjectDef.aabb.SetZero();

    const Mat3x4 &worldMatrix = GetEntity()->GetTransform()->GetMatrix();

    bool simulationEnded = true;
    
    for (int stageIndex = 0; stageIndex < renderObjectDef.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = renderObjectDef.particleSystem->GetStage(stageIndex);
        const ParticleSystem::StandardModule &standardModule = stage->standardModule;

        int simulationTime = standardModule.simulationSpeed * currentTime - SEC2MILLI(renderObjectDef.stageStartDelay[stageIndex]);
        if (simulationTime < 0) {
            // in delay time
            simulationEnded = false;
            continue;
        }

        int cycleDuration = standardModule.lifeTime + standardModule.deadTime;
        int curCycles = simulationTime / cycleDuration;
        bool stageEnded = false;

        if (!standardModule.looping) {
            if (curCycles > standardModule.maxCycles) {
                stageEnded = true;
            }
        }

        if (IsStopEmitting()) {
            if (simulationTime - stopEmitTime > cycleDuration) {
                stageEnded = true;
            }
        }

        if (!stageEnded) {
            simulationEnded = false;
        }

        int inCycleTime = simulationTime - curCycles * cycleDuration;

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) ? stage->trailsModule.count : 0;

        for (int particleIndex = 0; particleIndex < standardModule.count; particleIndex++) {
            int particleGenTime = standardModule.lifeTime * standardModule.spawnBunching * particleIndex / standardModule.count;
            int particleAge = inCycleTime - particleGenTime;

            // Wrap elapsed time of this particle if it is needed.
            if (particleAge <= 0) {
                if (standardModule.prewarm || curCycles > 0) {
                    particleAge += cycleDuration;
                }
            }

            int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;

            // Get the particle pointer with the given particle index.
            Particle *particle = (Particle *)((byte *)renderObjectDef.stageParticles[stageIndex] + particleIndex * particleSize);

            // Check this particle is alive now.
            if (!stageEnded && particleAge >= 0 && particleAge < standardModule.lifeTime) {
                // Generate if this particle is not generated yet. 
                bool regenerate = !particle->generated;

                if (curCycles > particle->cycle) {
                    if (inCycleTime > particleGenTime) {
                        if (!standardModule.looping && curCycles >= standardModule.maxCycles) {
                            particle->alive = false;
                            continue;
                        }

                        particle->cycle = curCycles;

                        regenerate = true;
                    }

                    if (curCycles - particle->cycle > 1) {
                        particle->cycle = curCycles - 1;

                        regenerate = true;
                    }
                }

                if (IsStopEmitting()) {
                    if (particle->cycle * cycleDuration + particleGenTime > stopEmitTime) {
                        continue;
                    }
                }

                particle->alive = true;

                if (regenerate) {
                    if (standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
                        particle->worldMatrix = worldMatrix;
                    }

                    InitializeParticle(particle, stage, (float)inCycleTime / cycleDuration);
                }

                ProcessTrail(particle, stage, particleAge);
            } else {
                particle->alive = false;
                particle->generated = false;
                particle->cycle = 0;
            }
        }
    }

    if (simulationEnded) {
        simulationStarted = false;
        stopEmitTime = 0;
        return;
    }

    ComRenderable::UpdateVisuals();
}

void ComParticleSystem::InitializeParticle(Particle *particle, const ParticleSystem::Stage *stage, float inCycleFrac) const {
    particle->generated = true;

    particle->initialSpeed = MeterToUnit(stage->standardModule.startSpeed.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac));

    particle->initialSize = MeterToUnit(stage->standardModule.startSize.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac));

    particle->initialAspectRatio = stage->standardModule.startAspectRatio.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac);

    particle->initialAngle = stage->standardModule.startRotation.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac);
    particle->initialAngle += RANDOM_FLOAT(-180, 180) * stage->standardModule.randomizeRotation;

    particle->initialColor = stage->standardModule.startColor; //

    if (stage->moduleFlags & (BIT(ParticleSystem::ModuleBit::LTSize) | BIT(ParticleSystem::ModuleBit::SizeBySpeed))) {
        particle->randomSize = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTAspectRatio)) {
        particle->randomAspectRatio = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & (BIT(ParticleSystem::ModuleBit::LTRotation) | BIT(ParticleSystem::ModuleBit::RotationBySpeed))) {
        particle->randomAngularVelocity = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTSpeed)) {
        particle->randomSpeed = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTForce)) {
        particle->randomForce.x = RANDOM_FLOAT(0, 1);
        particle->randomForce.y = RANDOM_FLOAT(0, 1);
        particle->randomForce.z = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Shape)) {
        const ParticleSystem::ShapeModule &shapeModule = stage->shapeModule;

        if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::Box) {
            particle->initialPosition.x = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.x, shapeModule.extents.x));
            particle->initialPosition.y = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.y, shapeModule.extents.y));
            particle->initialPosition.z = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.z, shapeModule.extents.z));

            if (shapeModule.randomizeDir == 0) {
                particle->direction = Vec3::unitZ;
            } else {
                Vec3 randomDir = Vec3::FromUniformSampleSphere(RANDOM_FLOAT(0, 1), RANDOM_FLOAT(0, 1));

                particle->direction = Math::Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::Sphere) {
            float r = MeterToUnit(shapeModule.radius);
            if (shapeModule.thickness > 0) {
                r = RANDOM_FLOAT(r * (1.0f - shapeModule.thickness), r);
            }

            particle->initialPosition = Vec3::FromUniformSampleSphere(RANDOM_FLOAT(0, 1), RANDOM_FLOAT(0, 1));
            particle->initialPosition *= r;

            if (shapeModule.randomizeDir == 0) {
                particle->direction = Vec3::unitZ;
            } else {
                Vec3 randomDir = Vec3::FromUniformSampleSphere(RANDOM_FLOAT(0, 1), RANDOM_FLOAT(0, 1));

                particle->direction = Math::Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::Circle) {
            float r = MeterToUnit(shapeModule.radius);
            if (shapeModule.thickness > 0) {
                r = RANDOM_FLOAT(r * (1.0f - shapeModule.thickness), r);
            }

            particle->initialPosition.ToVec2() = Vec2::FromUniformSampleCircle(RANDOM_FLOAT(0, 1));
            particle->initialPosition.z = 0;
            particle->initialPosition *= r;

            if (shapeModule.randomizeDir == 0) {
                particle->direction = Vec3::unitZ;
            } else {
                Vec3 randomDir = Vec3::FromUniformSampleSphere(RANDOM_FLOAT(0, 1), RANDOM_FLOAT(0, 1));

                particle->direction = Math::Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::Cone) {
            float r = MeterToUnit(shapeModule.radius);
            if (shapeModule.thickness > 0) {
                r = RANDOM_FLOAT(r * (1.0f - shapeModule.thickness), r);
            }

            Vec2 p = Vec2::FromUniformSampleCircle(RANDOM_FLOAT(0, 1));
            particle->initialPosition.ToVec2() = p;
            particle->initialPosition.z = 0;
            particle->initialPosition *= r;

            particle->direction = Vec3::unitZ;

            if (r > FLT_EPSILON) {
                float l2 = particle->initialPosition.LengthSqr();

                if (l2 > FLT_EPSILON) {
                    float angleScale = l2 / (r * r);
                    if (shapeModule.randomizeDir > 0) {
                        angleScale = Math::Lerp(angleScale, RANDOM_FLOAT(-1.f, 1.f), shapeModule.randomizeDir);
                    }

                    float rotAngle = shapeModule.angle * angleScale;
                    Vec3 rotDir = Vec3(-p.y, p.x, 0);
                    Rotation rotation(Vec3::origin, rotDir, rotAngle);
                    particle->direction = rotation.RotatePoint(particle->direction);
                }
            }
        }
    } else {
        particle->initialPosition.Set(0, 0, 0);

        particle->direction.Set(0, 0, 0);
    }    
}

void ComParticleSystem::ProcessTrail(Particle *particle, const ParticleSystem::Stage *stage, int particleAge) {
    ALIGN_AS32 Mat3x4 offsetMatrix;

    if (stage->standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
        offsetMatrix = GetEntity()->GetTransform()->GetMatrix().InverseOrthogonal() * particle->worldMatrix;
    }
    
    int trailCount = (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) ? stage->trailsModule.count : 0;
    int pivotCount = 1 + trailCount;

    for (int pivotIndex = 0; pivotIndex < pivotCount; pivotIndex++) {
        Particle::Trail *trail = &particle->trails[pivotIndex];
        
        float trailAge = particleAge;

        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) {
            trailAge -= (stage->standardModule.lifeTime * stage->trailsModule.length) * pivotIndex / trailCount;
            
            if (stage->trailsModule.trailCut) {
                if (trailAge < 0) {
                    trailAge = 0;
                }
            }
        }

        float trailFrac = trailAge / stage->standardModule.lifeTime;

        float trailSpeed;
        
        if (stage->moduleFlags & (BIT(ParticleSystem::ModuleBit::SizeBySpeed) | BIT(ParticleSystem::ModuleBit::RotationBySpeed))) {
            if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::CustomPath)) {
                trailSpeed = 0;
            } else if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTSpeed)) {
                trailSpeed = particle->initialSpeed + MeterToUnit(stage->speedOverLifetimeModule.speed.Evaluate(particle->randomSpeed, trailFrac));
            } else {
                trailSpeed = particle->initialSpeed;
            }
        }

        // Compute size.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTSize)) {
            trail->size = particle->initialSize * stage->sizeOverLifetimeModule.size.Evaluate(particle->randomSize, trailFrac);
        } else if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::SizeBySpeed)) {
            float l = Math::Fabs(stage->sizeBySpeedModule.speedRange[1] - stage->sizeBySpeedModule.speedRange[0]);
            float speedFrac = (UnitToMeter(trailSpeed) - stage->sizeBySpeedModule.speedRange[0]) / l;
            trail->size = particle->initialSize * stage->sizeBySpeedModule.size.Evaluate(particle->randomSize, speedFrac);
        } else {
            trail->size = particle->initialSize;
        }

        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::Trails)) {
            trail->size *= Math::Lerp(1.0f, stage->trailsModule.trailScale, (float)pivotIndex / trailCount);
        }

        // Compute aspect ratio.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTAspectRatio)) {
            trail->aspectRatio = particle->initialAspectRatio * stage->aspectRatioOverLifetimeModule.aspectRatio.Evaluate(particle->randomAspectRatio, trailFrac);
        } else {
            trail->aspectRatio = particle->initialAspectRatio;
        }

        // Compute rotation angle.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTRotation)) {
            float angularVelocity = stage->rotationOverLifetimeModule.rotation.Evaluate(particle->randomAngularVelocity, trailFrac);
            trail->angle = particle->initialAngle + MILLI2SEC(trailAge) * angularVelocity;
        } else if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::RotationBySpeed)) {
            float l = Math::Fabs(stage->rotationBySpeedModule.speedRange[1] - stage->rotationBySpeedModule.speedRange[0]);
            float speedFrac = (UnitToMeter(trailSpeed) - stage->rotationBySpeedModule.speedRange[0]) / l;
            float angularVelocity = stage->rotationBySpeedModule.rotation.Evaluate(particle->randomSize, speedFrac);
            trail->angle = particle->initialAngle + MILLI2SEC(trailAge) * angularVelocity; 
        } else {
            trail->angle = particle->initialAngle;
        }
        
        // Compute color.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTColor)) {
            if (trailFrac < stage->colorOverLifetimeModule.fadeLocation) {
                // fade in.
                float f = trailFrac / stage->colorOverLifetimeModule.fadeLocation;
                trail->color = Math::Lerp(stage->colorOverLifetimeModule.targetColor, particle->initialColor, f);
            } else { 
                // fade out.
                float f = (trailFrac - stage->colorOverLifetimeModule.fadeLocation) / (1.f - stage->colorOverLifetimeModule.fadeLocation);
                trail->color = Math::Lerp(particle->initialColor, stage->colorOverLifetimeModule.targetColor, f);
            }
        } else {
            trail->color = particle->initialColor;
        }

        // Compute position.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::CustomPath)) {
            ComputeTrailPositionFromCustomPath(stage->customPathModule, particle, trailFrac, trail);
        } else if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTSpeed)) {
            float dist = particle->initialSpeed * trailFrac + MeterToUnit(stage->speedOverLifetimeModule.speed.Integrate(particle->randomSpeed, trailFrac));

            trail->position = particle->initialPosition + particle->direction * dist;
        } else {
            float dist = particle->initialSpeed * trailFrac;

            trail->position = particle->initialPosition + particle->direction * dist;
        }

        // Apply force.
        if (stage->moduleFlags & BIT(ParticleSystem::ModuleBit::LTForce)) {
            Vec3 force(
                MeterToUnit(stage->forceOverLifetimeModule.force[0].Evaluate(particle->randomForce.x, trailFrac)),
                MeterToUnit(stage->forceOverLifetimeModule.force[1].Evaluate(particle->randomForce.y, trailFrac)),
                MeterToUnit(stage->forceOverLifetimeModule.force[2].Evaluate(particle->randomForce.z, trailFrac)));
        
            trail->position += force * 0.5f * trailFrac * trailFrac;
        }

        // Apply gravity.
        trail->position.z -= MeterToUnit(stage->standardModule.gravity) * 0.5f * trailFrac * trailFrac;

        if (stage->standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
            trail->position = offsetMatrix.TransformPos(trail->position);
        }

        // Add trail bounds to the entity bounds.
        float radius;
        if (stage->standardModule.orientation == ParticleSystem::StandardModule::Orientation::Aimed || 
            stage->standardModule.orientation == ParticleSystem::StandardModule::Orientation::AimedZ) {
            radius = trail->size * 0.5f * 2.0f;
        } else {
            radius = trail->size * 0.5f;
        }

        renderObjectDef.aabb.AddAABB(Sphere(trail->position, radius).ToAABB());
    }
}

void ComParticleSystem::ComputeTrailPositionFromCustomPath(const ParticleSystem::CustomPathModule &customPathModule, const Particle *particle, float t, Particle::Trail *trail) const {
    if (customPathModule.customPath == ParticleSystem::CustomPathModule::CustomPath::Cone) {
        float radialTheta = t * DEG2RAD(customPathModule.radialSpeed);
        float s, c;
        Math::SinCos(radialTheta, s, c);
        c = c * (1.0f - t);
        s = s * (1.0f - t);

        trail->position.x = particle->initialPosition.x * c + particle->initialPosition.y * s;
        trail->position.y = particle->initialPosition.y * c - particle->initialPosition.x * s;
        trail->position.z = 0;
        return;
    }
    
    if (customPathModule.customPath == ParticleSystem::CustomPathModule::CustomPath::Helix) {
        float radialTheta = t * DEG2RAD(customPathModule.radialSpeed);
        float s, c;
        Math::SinCos(radialTheta, s, c);

        trail->position.x = particle->initialPosition.x * c + particle->initialPosition.y * s;
        trail->position.y = particle->initialPosition.y * c - particle->initialPosition.x * s;
        trail->position.z = particle->initialPosition.z + t * particle->direction.z;
        return;
    }

    if (customPathModule.customPath == ParticleSystem::CustomPathModule::CustomPath::Spherical) {
        float radialTheta = t * DEG2RAD(customPathModule.radialSpeed);
        float axialTheta = t * customPathModule.axialSpeed;
        float s, c;
        Math::SinCos(radialTheta, s, c);

        Vec3 rotDir = Vec3::unitZ.Cross(particle->initialPosition.Normalized());
        Rotation rotation(Vec3::origin, rotDir, axialTheta);
        Vec3 vec = rotation.RotatePoint(particle->initialPosition);

        trail->position.x = vec.x * c + vec.y * s;
        trail->position.y = vec.y * c - vec.x * s;
        trail->position.z = vec.z;
        return;
    }

    assert(0);
}

#if WITH_EDITOR
void ComParticleSystem::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    // Fade icon alpha in near distance
    float alpha = Clamp(spriteDef.worldMatrix.ToTranslationVec3().Distance(camera->GetState().origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}
#endif

bool ComParticleSystem::IsAlive() const {
    return simulationStarted;
}

void ComParticleSystem::Play() {
    ClearParticles();

    currentTime = 0;
    stopEmitTime = 0;
    simulationStarted = true;
}

void ComParticleSystem::Stop(StopMode::Enum stopMode) {
    if (stopMode == StopMode::Enum::StopEmitting) {
        if (!IsStopEmitting()) {
            stopEmitTime = currentTime;
        }
    } else if (stopMode == StopMode::Enum::StopEmittingAndClear) {
        ClearParticles();

        currentTime = 0;
        stopEmitTime = 0;
        simulationStarted = false;
    }
}

void ComParticleSystem::Resume() {
    simulationStarted = true;
}

void ComParticleSystem::Pause() {
    simulationStarted = false;
}

void ComParticleSystem::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

#if WITH_EDITOR
    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddRenderObject(&spriteDef);
    } else {
        renderWorld->UpdateRenderObject(spriteHandle, &spriteDef);
    }
#endif

    ComRenderable::UpdateVisuals();
}

void ComParticleSystem::TransformUpdated(const ComTransform *transform) {
#if WITH_EDITOR
    spriteDef.worldMatrix.SetTranslation(transform->GetOrigin());
#endif

    UpdateVisuals();
}

void ComParticleSystem::ParticleSystemReloaded() {
    SetParticleSystemGuid(GetProperty("particleSystem").As<Guid>());
}

Guid ComParticleSystem::GetParticleSystemGuid() const {
    if (renderObjectDef.particleSystem) {
        const Str particleSystemPath = renderObjectDef.particleSystem->GetHashName();
        return resourceGuidMapper.Get(particleSystemPath);
    }
    return Guid();
}

void ComParticleSystem::SetParticleSystemGuid(const Guid &guid) {
    ChangeParticleSystem(guid);

    UpdateVisuals();
}

ParticleSystem *ComParticleSystem::GetParticleSystem() const {
    Guid particleSystemGuid = GetParticleSystemGuid();
    if (particleSystemGuid.IsZero()) {
        return nullptr;
    }

    const Str particleSystemHashName = resourceGuidMapper.Get(particleSystemGuid);
    return particleSystemManager.GetParticleSystem(particleSystemHashName);
}

void ComParticleSystem::SetParticleSystem(const ParticleSystem *particleSystem) {
    const Guid particleSystemGuid = resourceGuidMapper.Get(particleSystem->GetHashName());

    SetParticleSystemGuid(particleSystemGuid);
}

BE_NAMESPACE_END
