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
#include "Components/ComTransform.h"
#include "Components/ComParticleSystem.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Particle System", ComParticleSystem, ComRenderable)
BEGIN_EVENTS(ComParticleSystem)
END_EVENTS

void ComParticleSystem::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("particleSystem", "Particle System", Guid, GetParticleSystemGuid, SetParticleSystemGuid, GuidMapper::defaultParticleSystemGuid, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&ParticleSystemAsset::metaObject);
    REGISTER_PROPERTY("playOnAwake", "Play On Awake", bool, playOnAwake, true, "", PropertyInfo::EditorFlag);
}

ComParticleSystem::ComParticleSystem() {
    particleSystemAsset = nullptr;

    spriteHandle = -1;
    spriteReferenceMesh = nullptr;
    memset(&sprite, 0, sizeof(sprite));
}

ComParticleSystem::~ComParticleSystem() {
    Purge(false);
}

void ComParticleSystem::Purge(bool chainPurge) {
    if (sceneEntity.particleSystem) {
        particleSystemManager.ReleaseParticleSystem(sceneEntity.particleSystem);
        sceneEntity.particleSystem = nullptr;
    }

    if (sceneEntity.stageParticles.Count() > 0) {
        for (int stageIndex = 0; stageIndex < sceneEntity.stageParticles.Count(); stageIndex++) {
            Mem_Free(sceneEntity.stageParticles[stageIndex]);
        }

        sceneEntity.stageParticles.Clear();
    }

    if (sprite.mesh) {
        meshManager.ReleaseMesh(sprite.mesh);
        sprite.mesh = nullptr;
    }

    if (spriteReferenceMesh) {
        meshManager.ReleaseMesh(spriteReferenceMesh);
        spriteReferenceMesh = nullptr;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveEntity(spriteHandle);
        spriteHandle = -1;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComParticleSystem::Init() {
    ComRenderable::Init();

    currentTime = 0;
    stopTime = 0;

    simulationStarted = false;

    // 3d sprite
    spriteReferenceMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&sprite, 0, sizeof(sprite));
    sprite.layer = TagLayerSettings::EditorLayer;
    sprite.maxVisDist = MeterToUnit(50);
    sprite.billboard = true;

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/ParticleSystem.png", Texture::Clamp | Texture::HighQuality);
    sprite.materials.SetCount(1);
    sprite.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);

    sprite.mesh = spriteReferenceMesh->InstantiateMesh(Mesh::StaticMesh);
    sprite.aabb = spriteReferenceMesh->GetAABB();
    sprite.origin = GetEntity()->GetTransform()->GetOrigin();
    sprite.scale = Vec3(1, 1, 1);
    sprite.axis = Mat3::identity;
    sprite.materialParms[SceneEntity::RedParm] = 1.0f;
    sprite.materialParms[SceneEntity::GreenParm] = 1.0f;
    sprite.materialParms[SceneEntity::BlueParm] = 1.0f;
    sprite.materialParms[SceneEntity::AlphaParm] = 1.0f;
    sprite.materialParms[SceneEntity::TimeOffsetParm] = 0.0f;
    sprite.materialParms[SceneEntity::TimeScaleParm] = 1.0f;

    GetEntity()->GetTransform()->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComParticleSystem::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComParticleSystem::ChangeParticleSystem(const Guid &particleSystemGuid) {
    // Disconnect with previously connected particleSystem asset
    if (particleSystemAsset) {
        particleSystemAsset->Disconnect(&Asset::SIG_Reloaded, this);
        particleSystemAsset = nullptr;
    }

    // Release the previously used particleSystem
    if (sceneEntity.particleSystem) {
        particleSystemManager.ReleaseParticleSystem(sceneEntity.particleSystem);
        sceneEntity.particleSystem = nullptr;
    }

    // Get the new particleSystem
    const Str particleSystemPath = resourceGuidMapper.Get(particleSystemGuid);
    sceneEntity.particleSystem = particleSystemManager.GetParticleSystem(particleSystemPath);

    ResetParticles();

    // Need to particleSystem asset to be reloaded in editor
    particleSystemAsset = (ParticleSystemAsset *)ParticleSystemAsset::FindInstance(particleSystemGuid);
    if (particleSystemAsset) {
        particleSystemAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComParticleSystem::ParticleSystemReloaded, SignalObject::Queued);
    }
}

void ComParticleSystem::ResetParticles() {
    sceneEntity.stageStartDelay.SetCount(sceneEntity.particleSystem->NumStages());

    // Free memory used for particles
    if (sceneEntity.stageParticles.Count() > 0) {
        for (int stageIndex = 0; stageIndex < sceneEntity.stageParticles.Count(); stageIndex++) {
            Mem_Free(sceneEntity.stageParticles[stageIndex]);
        }

        sceneEntity.stageParticles.Clear();
    }

    sceneEntity.stageParticles.SetCount(sceneEntity.particleSystem->NumStages());

    for (int stageIndex = 0; stageIndex < sceneEntity.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = sceneEntity.particleSystem->GetStage(stageIndex);

        sceneEntity.stageStartDelay[stageIndex] = stage->standardModule.startDelay.Evaluate(RANDOM_FLOAT(0, 1), 0);

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage->trailsModule.count : 0;
        int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;
        int size = stage->standardModule.count * particleSize;

        sceneEntity.stageParticles[stageIndex] = (Particle *)Mem_Alloc(size);
        memset(sceneEntity.stageParticles[stageIndex], 0, size);
    }
}

void ComParticleSystem::Awake() {
    if (playOnAwake) {
        simulationStarted = true;
    }
}

void ComParticleSystem::OnActive() {
    ResetParticles();

    ComRenderable::OnActive();
}

void ComParticleSystem::OnInactive() {
    if (spriteHandle != -1) {
        renderWorld->RemoveEntity(spriteHandle);
        spriteHandle = -1;
    }

    ComRenderable::OnInactive();
}

bool ComParticleSystem::HasRenderEntity(int renderEntityHandle) const {
    if (this->spriteHandle == renderEntityHandle) {
        return true;
    }

    return ComRenderable::HasRenderEntity(renderEntityHandle);
}

int ComParticleSystem::GetAliveParticleCount() const {
    int aliveCount = 0;

    for (int stageIndex = 0; stageIndex < sceneEntity.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = sceneEntity.particleSystem->GetStage(stageIndex);
        const ParticleSystem::StandardModule &standardModule = stage->standardModule;

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage->trailsModule.count : 0;

        for (int particleIndex = 0; particleIndex < standardModule.count; particleIndex++) {
            int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;

            // Get the particle pointer with the given particle index
            Particle *particle = (Particle *)((byte *)sceneEntity.stageParticles[stageIndex] + particleIndex * particleSize);

            if (particle->alive) {
                aliveCount++;
            }
        }
    }

    return aliveCount;
}

void ComParticleSystem::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (!simulationStarted) {
        return;
    }

    int elapsedTime = GetGameWorld()->GetTime() - GetGameWorld()->GetPrevTime();

    currentTime += elapsedTime;

    UpdateSimulation(currentTime);
}

void ComParticleSystem::UpdateSimulation(int currentTime) {
    float time = MS2SEC(currentTime);

    sceneEntity.time = currentTime;

    sceneEntity.aabb.SetZero();

    const Mat3x4 worldMatrix = GetEntity()->GetTransform()->GetTransform();

    bool simulationEnded = true;
    
    for (int stageIndex = 0; stageIndex < sceneEntity.particleSystem->NumStages(); stageIndex++) {
        const ParticleSystem::Stage *stage = sceneEntity.particleSystem->GetStage(stageIndex);

        // Standard module
        const ParticleSystem::StandardModule &standardModule = stage->standardModule;

        // Is in delay time ?
        float simulationTime = standardModule.simulationSpeed * time - sceneEntity.stageStartDelay[stageIndex];
        if (simulationTime < 0) {
            simulationEnded = false;
            continue;
        }

        float cycleDuration = standardModule.lifeTime + standardModule.deadTime;

        int curCycles = (int)(simulationTime / cycleDuration);

        if (!standardModule.looping) {
            if (curCycles > standardModule.maxCycles) {
                continue;
            }
        }

        if (stopTime != 0) {
            if (time > MS2SEC(stopTime) + cycleDuration) {
                continue;
            }
        }

        simulationEnded = false;

        float inCycleTime = simulationTime - curCycles * cycleDuration;

        int trailCount = (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage->trailsModule.count : 0;

        for (int particleIndex = 0; particleIndex < standardModule.count; particleIndex++) {
            float particleGenTime = standardModule.lifeTime * standardModule.spawnBunching * particleIndex / standardModule.count;
            float particleAge = inCycleTime - particleGenTime;

            // Wrap elapsed time of this particle if it is needed
            if (particleAge <= 0) {
                if (standardModule.prewarm || curCycles > 0) {
                    particleAge += cycleDuration;
                }
            }

            int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;

            // Get the particle pointer with the given particle index
            Particle *particle = (Particle *)((byte *)sceneEntity.stageParticles[stageIndex] + particleIndex * particleSize);

            // Check this particle is alive now 
            if (particleAge >= 0 && particleAge < standardModule.lifeTime) {
                // Generate if this particle is not generated yet. 
                bool regenerate = !particle->generated;

                if (curCycles > particle->cycle) {
                    if (inCycleTime > particleGenTime) {
                        if (!standardModule.looping) {
                            if (curCycles >= standardModule.maxCycles) {
                                particle->alive = false;
                                continue;
                            }
                        }

                        particle->cycle = curCycles;

                        regenerate = true;
                    }

                    if (curCycles - particle->cycle > 1) {
                        particle->cycle = curCycles - 1;

                        regenerate = true;
                    }
                }

                if (stopTime > 0) {
                    if (particleGenTime + particle->cycle * cycleDuration > MS2SEC(stopTime)) {
                        continue;
                    }
                }

                particle->alive = true;

                if (regenerate) {
                    if (standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
                        particle->worldMatrix = worldMatrix;
                    }

                    InitializeParticle(particle, stage, inCycleTime / cycleDuration);
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
        stopTime = 0;
        return;
    }

    ComRenderable::UpdateVisuals();
}

void ComParticleSystem::InitializeParticle(Particle *particle, const ParticleSystem::Stage *stage, float inCycleFrac) const {
    particle->generated = true;

    particle->initialSpeed = MeterToUnit(stage->standardModule.startSpeed.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac));

    particle->initialSize = CentiToUnit(stage->standardModule.startSize.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac));

    particle->initialAspectRatio = stage->standardModule.startAspectRatio.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac);

    particle->initialAngle = stage->standardModule.startRotation.Evaluate(RANDOM_FLOAT(0, 1), inCycleFrac);
    particle->initialAngle += RANDOM_FLOAT(-180, 180) * stage->standardModule.randomizeRotation;

    particle->initialColor = stage->standardModule.startColor; //

    if (stage->moduleFlags & (BIT(ParticleSystem::LTSizeModuleBit) | BIT(ParticleSystem::SizeBySpeedModuleBit))) {
        particle->randomSize = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::LTAspectRatioModuleBit)) {
        particle->randomAspectRatio = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & (BIT(ParticleSystem::LTRotationModuleBit) | BIT(ParticleSystem::RotationBySpeedModuleBit))) {
        particle->randomAngularVelocity = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::LTSpeedModuleBit)) {
        particle->randomSpeed = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::LTForceModuleBit)) {
        particle->randomForce.x = RANDOM_FLOAT(0, 1);
        particle->randomForce.y = RANDOM_FLOAT(0, 1);
        particle->randomForce.z = RANDOM_FLOAT(0, 1);
    }

    if (stage->moduleFlags & BIT(ParticleSystem::ShapeModuleBit)) {
        const ParticleSystem::ShapeModule &shapeModule = stage->shapeModule;

        if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::BoxShape) {
            particle->initialPosition.x = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.x, shapeModule.extents.x));
            particle->initialPosition.y = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.y, shapeModule.extents.y));
            particle->initialPosition.z = MeterToUnit(RANDOM_FLOAT(-shapeModule.extents.z, shapeModule.extents.z));

            if (shapeModule.randomizeDir == 0) {
                particle->direction = Vec3::unitZ;
            } else {
                Vec3 randomDir = Vec3::FromUniformSampleSphere(RANDOM_FLOAT(0, 1), RANDOM_FLOAT(0, 1));

                particle->direction = Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::SphereShape) {
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

                particle->direction = Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::CircleShape) {
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

                particle->direction = Lerp(Vec3::unitZ, randomDir, shapeModule.randomizeDir);
            }
        } else if (shapeModule.shape == ParticleSystem::ShapeModule::Shape::ConeShape) {
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
                        angleScale = Lerp(angleScale, RANDOM_FLOAT(-1.f, 1.f), shapeModule.randomizeDir);
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

void ComParticleSystem::ProcessTrail(Particle *particle, const ParticleSystem::Stage *stage, float particleAge) {
    Mat3x4 offsetMatrix;
    if (stage->standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
        offsetMatrix = GetEntity()->GetTransform()->GetTransform().Inverse() * particle->worldMatrix;
    }
    
    int trailCount = (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage->trailsModule.count : 0;
    int pivotCount = 1 + trailCount;

    for (int pivotIndex = 0; pivotIndex < pivotCount; pivotIndex++) {
        Particle::Trail *trail = &particle->trails[pivotIndex];
        
        float trailAge = particleAge;

        if (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) {
            trailAge -= (stage->standardModule.lifeTime * stage->trailsModule.length) * pivotIndex / trailCount;
            
            if (stage->trailsModule.trailCut) {
                if (trailAge < 0) {
                    trailAge = 0;
                }
            }
        }

        float trailFrac = trailAge / stage->standardModule.lifeTime;

        float trailSpeed;
        
        if (stage->moduleFlags & (BIT(ParticleSystem::SizeBySpeedModuleBit) | BIT(ParticleSystem::RotationBySpeedModuleBit))) {
            if (stage->moduleFlags & BIT(ParticleSystem::CustomPathModuleBit)) {
                trailSpeed = 0;
            } else if (stage->moduleFlags & BIT(ParticleSystem::LTSpeedModuleBit)) {
                trailSpeed = particle->initialSpeed + MeterToUnit(stage->speedOverLifetimeModule.speed.Evaluate(particle->randomSpeed, trailFrac));
            } else {
                trailSpeed = particle->initialSpeed;
            }
        }

        // Compute size
        if (stage->moduleFlags & BIT(ParticleSystem::LTSizeModuleBit)) {
            trail->size = particle->initialSize * CentiToUnit(stage->sizeOverLifetimeModule.size.Evaluate(particle->randomSize, trailFrac));
        } else if (stage->moduleFlags & BIT(ParticleSystem::SizeBySpeedModuleBit)) {
            float l = Math::Fabs(stage->sizeBySpeedModule.speedRange[1] - stage->sizeBySpeedModule.speedRange[0]);
            float speedFrac = (UnitToMeter(trailSpeed) - stage->sizeBySpeedModule.speedRange[0]) / l;
            trail->size = particle->initialSize * CentiToUnit(stage->sizeBySpeedModule.size.Evaluate(particle->randomSize, speedFrac));
        } else {
            trail->size = particle->initialSize;
        }

        if (stage->moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) {
            trail->size *= Lerp(1.0f, stage->trailsModule.trailScale, (float)pivotIndex / trailCount);
        }

        // Compute aspect ratio
        if (stage->moduleFlags & BIT(ParticleSystem::LTAspectRatioModuleBit)) {
            trail->aspectRatio = particle->initialAspectRatio * stage->aspectRatioOverLifetimeModule.aspectRatio.Evaluate(particle->randomAspectRatio, trailFrac);
        } else {
            trail->aspectRatio = particle->initialAspectRatio;
        }

        // Compute rotation angle
        if (stage->moduleFlags & BIT(ParticleSystem::LTRotationModuleBit)) {
            float angularVelocity = stage->rotationOverLifetimeModule.rotation.Evaluate(particle->randomAngularVelocity, trailFrac);
            trail->angle = particle->initialAngle + trailAge * angularVelocity;
        } else if (stage->moduleFlags & BIT(ParticleSystem::RotationBySpeedModuleBit)) {
            float l = Math::Fabs(stage->rotationBySpeedModule.speedRange[1] - stage->rotationBySpeedModule.speedRange[0]);
            float speedFrac = (UnitToMeter(trailSpeed) - stage->rotationBySpeedModule.speedRange[0]) / l;
            float angularVelocity = stage->rotationBySpeedModule.rotation.Evaluate(particle->randomSize, speedFrac);
            trail->angle = particle->initialAngle + trailAge * angularVelocity; 
        } else {
            trail->angle = particle->initialAngle;
        }
        
        // Compute color
        if (stage->moduleFlags & BIT(ParticleSystem::LTColorModuleBit)) {
            if (trailFrac < stage->colorOverLifetimeModule.fadeLocation) {
                // fade in
                float f = trailFrac / stage->colorOverLifetimeModule.fadeLocation;
                trail->color = Lerp(stage->colorOverLifetimeModule.targetColor, particle->initialColor, f);
            } else { 
                // fade out
                float f = (trailFrac - stage->colorOverLifetimeModule.fadeLocation) / (1.f - stage->colorOverLifetimeModule.fadeLocation);
                trail->color = Lerp(particle->initialColor, stage->colorOverLifetimeModule.targetColor, f);
            }
        } else {
            trail->color = particle->initialColor;
        }

        // Compute position
        if (stage->moduleFlags & BIT(ParticleSystem::CustomPathModuleBit)) {
            ComputeTrailPositionFromCustomPath(stage->customPathModule, particle, trailFrac, trail);
        } else if (stage->moduleFlags & BIT(ParticleSystem::LTSpeedModuleBit)) {
            float dist = particle->initialSpeed * trailFrac + MeterToUnit(stage->speedOverLifetimeModule.speed.Integrate(particle->randomSpeed, trailFrac));

            trail->position = particle->initialPosition + particle->direction * dist;
        } else {
            float dist = particle->initialSpeed * trailFrac;

            trail->position = particle->initialPosition + particle->direction * dist;
        }

        // Apply force
        if (stage->moduleFlags & BIT(ParticleSystem::LTForceModuleBit)) {
            Vec3 force(
                MeterToUnit(stage->forceOverLifetimeModule.force[0].Evaluate(particle->randomForce.x, trailFrac)),
                MeterToUnit(stage->forceOverLifetimeModule.force[1].Evaluate(particle->randomForce.y, trailFrac)),
                MeterToUnit(stage->forceOverLifetimeModule.force[2].Evaluate(particle->randomForce.z, trailFrac)));
        
            trail->position += force * 0.5f * trailFrac * trailFrac;
        }

        // Apply gravity
        trail->position.z -= MeterToUnit(stage->standardModule.gravity) * 0.5f * trailFrac * trailFrac;

        if (stage->standardModule.simulationSpace == ParticleSystem::StandardModule::SimulationSpace::Global) {
            trail->position = offsetMatrix * trail->position;
        }

        // Add trail bounds to the entity bounds
        float radius;
        if (stage->standardModule.orientation == ParticleSystem::StandardModule::Orientation::Aimed || 
            stage->standardModule.orientation == ParticleSystem::StandardModule::Orientation::AimedZ) {
            radius = trail->size * 0.5f * 2.0f;
        } else {
            radius = trail->size * 0.5f;
        }

        sceneEntity.aabb.AddAABB(Sphere(trail->position, radius).ToAABB());
    }
}

void ComParticleSystem::ComputeTrailPositionFromCustomPath(const ParticleSystem::CustomPathModule &customPathModule, const Particle *particle, float t, Particle::Trail *trail) const {
    if (customPathModule.customPath == ParticleSystem::CustomPathModule::ConePath) {
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
    
    if (customPathModule.customPath == ParticleSystem::CustomPathModule::HelixPath) {
        float radialTheta = t * DEG2RAD(customPathModule.radialSpeed);
        float s, c;
        Math::SinCos(radialTheta, s, c);

        trail->position.x = particle->initialPosition.x * c + particle->initialPosition.y * s;
        trail->position.y = particle->initialPosition.y * c - particle->initialPosition.x * s;
        trail->position.z = particle->initialPosition.z + t * particle->direction.z;
        return;
    }

    if (customPathModule.customPath == ParticleSystem::CustomPathModule::SphericalPath) {
        float radialTheta = t * DEG2RAD(customPathModule.radialSpeed);
        float axialTheta = t * customPathModule.axialSpeed;
        float s, c;
        Math::SinCos(radialTheta, s, c);

        Vec3 tmp = particle->initialPosition;
        tmp.Normalize();
        Vec3 rotDir = Vec3::unitZ.Cross(tmp);
        Rotation rotation(Vec3::origin, rotDir, axialTheta);
        Vec3 vec = rotation.RotatePoint(particle->initialPosition);

        trail->position.x = vec.x * c + vec.y * s;
        trail->position.y = vec.y * c - vec.x * s;
        trail->position.z = vec.z;
        return;
    }

    assert(0);
}

void ComParticleSystem::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(sprite.origin.Distance(sceneView.origin) / MeterToUnit(8), 0.01f, 1.0f);

    sprite.materials[0]->GetPass()->constantColor[3] = alpha;
}

bool ComParticleSystem::IsAlive() const {
    return simulationStarted;
}

void ComParticleSystem::Play() {
    simulationStarted = true;
    currentTime = 0;
    stopTime = 0;
}

void ComParticleSystem::Stop() {
    if (stopTime == 0) {
        stopTime = currentTime;
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

    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddEntity(&sprite);
    } else {
        renderWorld->UpdateEntity(spriteHandle, &sprite);
    }

    ComRenderable::UpdateVisuals();
}

void ComParticleSystem::TransformUpdated(const ComTransform *transform) {
    sprite.origin = transform->GetOrigin();

    UpdateVisuals();
}

void ComParticleSystem::ParticleSystemReloaded() {
    SetParticleSystemGuid(GetProperty("particleSystem").As<Guid>());
}

Guid ComParticleSystem::GetParticleSystemGuid() const {
    if (sceneEntity.particleSystem) {
        const Str particleSystemPath = sceneEntity.particleSystem->GetHashName();
        return resourceGuidMapper.Get(particleSystemPath);
    }
    return Guid();
}

void ComParticleSystem::SetParticleSystemGuid(const Guid &guid) {
    ChangeParticleSystem(guid);

    UpdateVisuals();
}

BE_NAMESPACE_END
