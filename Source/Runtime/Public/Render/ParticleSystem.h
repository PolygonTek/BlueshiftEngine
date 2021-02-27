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

/*
-------------------------------------------------------------------------------

    Particle System

-------------------------------------------------------------------------------
*/

#include "Core/MinMaxCurve.h"
#include "Material.h"

BE_NAMESPACE_BEGIN

class ParticleMesh;

class Particle {
public:
    enum {
        MaxTrails = 32
    };

    struct Trail {
        Vec3                    position;
        float                   size;
        float                   angle;
        float                   aspectRatio;
        Color4                  color;
    };

    bool                        generated;
    bool                        alive;
    int                         cycle;

    Vec3                        direction;
    Mat3x4                      worldMatrix;

    Vec3                        initialPosition;
    float                       initialSpeed;
    float                       initialSize;
    float                       initialAspectRatio;
    float                       initialAngle;
    Color4                      initialColor;
    
    Vec3                        randomForce;            ///< Random seed for force [0, 1]
    float                       randomSpeed;            ///< Random seed for speed over lifetime [0, 1]
    float                       randomSize;             ///< Random seed for size over lifetime [0, 1]
    float                       randomAspectRatio;      ///< Random seed for aspect ratio over lifetime [0, 1]
    float                       randomAngularVelocity;  ///< Random seed for rotation over lifetime [0, 1]

    Trail                       trails[1];
};

class ParticleSystem {
    friend class ParticleMesh;
    friend class ParticleSystemManager;

public:
    struct ModuleBit {
        enum Enum {
            Standard            = 0,
            Shape               = 1,
            CustomPath          = 2,
            LTColor             = 3,
            LTSpeed             = 4,
            LTForce             = 5,
            LTRotation          = 6,
            RotationBySpeed     = 7,
            LTSize              = 8,
            SizeBySpeed         = 9,
            LTAspectRatio       = 10,
            Trails              = 11,
            //Collision         = 12,
            //Noise             = 13,
            //SubEmitters       = 14,
            //Light             = 15,
            Count
        };
    };    

    // standard module
    struct StandardModule {
        struct Orientation {
            enum Enum {
                View,                                   ///< Billboard
                ViewZ,                                  ///< Billboard aligned Z axis
                Aimed,
                AimedZ,
                X,                                      ///< Quad aligned X axis
                Y,                                      ///< Quad aligned Y axis
                Z                                       ///< Quad aligned Z axis
            };
        };

        struct SimulationSpace {
            enum Enum {
                Local,
                Global
            };
        };

        void                    Reset();

        int                     count;
        float                   spawnBunching;          ///< 0.0 = all come out at first instant, 1.0 = evenly spaced over life time
        int                     lifeTime;               ///< Total milliseconds of life for each particles
        int                     deadTime;               ///< Time after lifeTime before re-emit
        bool                    looping;
        bool                    prewarm;
        int                     maxCycles;
        
        float                   simulationSpeed;
        SimulationSpace::Enum   simulationSpace;

        Material *              material;
        bool                    animation;
        int                     animFrames[2];
        int                     animFps;
        Orientation::Enum       orientation;

        MinMaxCurve             startDelay;             ///< Delay from start for the first particle to emit 
        Color4                  startColor;             ///< Start color (RGBA) of particles
        MinMaxCurve             startSpeed;             ///< Start speed of particles in meter
        MinMaxCurve             startSize;              ///< Start size of particles in meter
        MinMaxCurve             startAspectRatio;       ///< Start aspect ratio of particles
        MinMaxCurve             startRotation;          ///< Start angle of particles in degrees
        float                   randomizeRotation;      ///< Bias [0, 1] of randomized angle
        float                   gravity;                ///< Can be nagative to float up
    };

    // shape module
    struct ShapeModule {
        struct Shape {
            enum Enum {
                Box,
                Sphere,
                Circle,
                Cone
            };
        };

        void                    Reset();

        Shape::Enum             shape;
        Vec3                    extents;                // used in Box
        float                   radius;                 // used in Sphere, Circle, Cone
        float                   thickness;              // used in Sphere, Circle, Cone
        float                   angle;                  // used in Cone
        float                   randomizeDir;           // [0, 1]
    };

    // custom path module
    struct CustomPathModule {
        struct CustomPath {
            enum Enum {
                Cone,
                Helix,
                Spherical
            };
        };

        void                    Reset();

        CustomPath::Enum        customPath;
        float                   radialSpeed;            // used in Cone, Helix, Spherical 
        float                   axialSpeed;             // used in Spherical
        float                   innerRadius;            // inner radius in meter
        float                   outerRadius;            // outer radius in meter
    };    
    
    // color over lifetime module
    struct LTColorModule {
        void                    Reset() { targetColor.Set(1, 1, 1, 0); fadeLocation = 0.5f; }

        Color4                  targetColor;
        float                   fadeLocation;
    };

    // speed over lifetime module
    struct LTSpeedModule {
        void                    Reset() { speed.Reset(MinMaxCurve::Type::Constant, 2.0f, 0.0f, 1.0f); }

        MinMaxCurve             speed;
    };

    // force module
    struct LTForceModule {
        void                    Reset();

        MinMaxCurve             force[3];               ///< X, Y, Z
    };

    // rotation over lifetime module
    struct LTRotationModule {
        void                    Reset() { rotation.Reset(MinMaxCurve::Type::Constant, 180, 0.0f, 0.0f); }

        MinMaxCurve             rotation;               ///< angular velocity
    };

    // rotation by speed module
    struct RotationBySpeedModule {
        void                    Reset() { rotation.Reset(MinMaxCurve::Type::Curve, 180.0f, 0.0f, 0.0f); speedRange.Set(0, 1); }

        MinMaxCurve             rotation;
        Vec2                    speedRange;
    };

    // size over lifetime module
    struct LTSizeModule {
        void                    Reset() { size.Reset(MinMaxCurve::Type::Constant, 1.0f, 0.0f, 1.0f); }

        MinMaxCurve             size;                   ///< size in meter
    };

    // size by speed module
    struct SizeBySpeedModule {
        void                    Reset() { size.Reset(MinMaxCurve::Type::Curve, 1.0f, 0.0f, 1.0f); speedRange.Set(0, 1); }

        MinMaxCurve             size;                   ///< size in meter
        Vec2                    speedRange;
    };

    // aspect ratio over lifetime module
    struct LTAspectRatioModule {
        void                    Reset() { aspectRatio.Reset(MinMaxCurve::Type::Constant, 1.0f, 1.0f, 1.0f); }

        MinMaxCurve             aspectRatio;
    };

    // trails module
    struct TrailsModule {
        void                    Reset() { count = 1; length = 0.1f; trailScale = 1.0f, trailCut = true; }

        int                     count;
        float                   length;                 // [0.0, 1.0]
        float                   trailScale;
        bool                    trailCut;
    };

    struct Stage {
        void                    Reset();

        Str                     name;
        bool                    skipRender;             // used for Editor
        int                     moduleFlags;
        StandardModule          standardModule;
        ShapeModule             shapeModule;
        CustomPathModule        customPathModule;
        LTColorModule           colorOverLifetimeModule;
        LTSpeedModule           speedOverLifetimeModule;
        LTForceModule           forceOverLifetimeModule;
        LTRotationModule        rotationOverLifetimeModule;
        RotationBySpeedModule   rotationBySpeedModule;
        LTSizeModule            sizeOverLifetimeModule;
        SizeBySpeedModule       sizeBySpeedModule;
        LTAspectRatioModule     aspectRatioOverLifetimeModule;
        TrailsModule            trailsModule;
    };

    ParticleSystem();
    ~ParticleSystem();

    const char *                GetName() const { return name; }
    const char *                GetHashName() const { return hashName; }

    void                        Purge();

    bool                        Create(const char *text);

    void                        CreateDefaultParticleSystem();

    int                         NumStages() const { return stages.Count(); }
    Stage *                     GetStage(int stageIndex) { return &stages[stageIndex]; }
    void                        AddStage();
    bool                        RemoveStage(int stageIndex);
    void                        SwapStages(int stageIndex0, int stageIndex1);

    bool                        Load(const char *filename);
    bool                        Reload();
    void                        Write(const char *filename);

    const ParticleSystem *      AddRefCount() const { refCount++; return this; }
    int                         GetRefCount() const { return refCount; }

private:
    bool                        ParseStage(Lexer &lexer, Stage &stage) const;
    bool                        ParseStandardModule(Lexer &lexer, StandardModule &module) const;
    bool                        ParseSimulationSpace(Lexer &lexer, StandardModule::SimulationSpace::Enum *simulationSpace) const;
    bool                        ParseOrientation(Lexer &lexer, StandardModule::Orientation::Enum *orientation) const;
    bool                        ParseMinMaxCurve(Lexer &lexer, MinMaxCurve *var) const;
    bool                        ParseTimeWrapMode(Lexer &lexer, Hermite<float>::TimeWrapMode *timeWrapMode) const;
    bool                        ParseShapeModule(Lexer &lexer, ShapeModule &module) const;
    bool                        ParseShape(Lexer &lexer, ShapeModule::Shape::Enum *shape) const;
    bool                        ParseCustomPathModule(Lexer &lexer, CustomPathModule &module) const;
    bool                        ParseCustomPath(Lexer &lexer, CustomPathModule::CustomPath::Enum *customPath) const;
    bool                        ParseLTForceModule(Lexer &lexer, LTForceModule &module) const;
    bool                        ParseLTColorModule(Lexer &lexer, LTColorModule &module) const;
    bool                        ParseLTSpeedModule(Lexer &lexer, LTSpeedModule &module) const;
    bool                        ParseLTRotationModule(Lexer &lexer, LTRotationModule &module) const;
    bool                        ParseRotationBySpeedModule(Lexer &lexer, RotationBySpeedModule &module) const;
    bool                        ParseLTSizeModule(Lexer &lexer, LTSizeModule &module) const;
    bool                        ParseSizeBySpeedModule(Lexer &lexer, SizeBySpeedModule &module) const;
    bool                        ParseLTAspectRatioModule(Lexer &lexer, LTAspectRatioModule &module) const;
    bool                        ParseTrailsModule(Lexer &lexer, TrailsModule &module) const;

    Str                         hashName;
    Str                         name;
    mutable int                 refCount = 0;
    bool                        permanence = false;

    Array<Stage>                stages;
};

BE_INLINE ParticleSystem::ParticleSystem() {
}

BE_INLINE ParticleSystem::~ParticleSystem() {
    Purge();
}

BE_INLINE void ParticleSystem::StandardModule::Reset() {
    lifeTime = 2000;
    deadTime = 0;
    looping = true;
    prewarm = true;
    maxCycles = 1;
    count = 32;
    spawnBunching = 1.0f;
    simulationSpeed = 1.0f;
    simulationSpace = SimulationSpace::Local;
    material = materialManager.defaultMaterial;
    animation = false;
    animFrames[0] = 1;
    animFrames[1] = 1;
    animFps = 8;
    orientation = Orientation::View;
    startDelay.Reset(MinMaxCurve::Type::Constant, 1.0f, 0.0f, 0.0f);
    startColor.Set(1, 1, 1, 1);
    startSpeed.Reset(MinMaxCurve::Type::Constant, 1.0f, 1.0f, 1.0f);
    startSize.Reset(MinMaxCurve::Type::Constant, 1.0f, 0.1f, 0.1f);
    startAspectRatio.Reset(MinMaxCurve::Type::Constant, 1.0f, 1.0f, 1.0f);
    startRotation.Reset(MinMaxCurve::Type::Constant, 180.0f, 0.0f, 0.0f);
    randomizeRotation = 1.0f;
    gravity = 0.0f;
}

BE_INLINE void ParticleSystem::ShapeModule::Reset() {
    shape = Shape::Cone;
    extents = Vec3::zero;
    radius = 0.1f;
    thickness = 1.0f;
    angle = 30;
    randomizeDir = 0.0f;
}

BE_INLINE void ParticleSystem::CustomPathModule::Reset() {
    customPath = CustomPath::Cone;
    radialSpeed = 180;
    axialSpeed = 90;
    innerRadius = 0.1f;
    outerRadius = 1.0f;
}

BE_INLINE void ParticleSystem::LTForceModule::Reset() {
    force[0].Reset(MinMaxCurve::Type::Constant, 1.0f, 0.0f, 0.0f); 
    force[1].Reset(MinMaxCurve::Type::Constant, 1.0f, 0.0f, 0.0f); 
    force[2].Reset(MinMaxCurve::Type::Constant, 1.0f, 0.0f, 0.0f);
}

BE_INLINE void ParticleSystem::Stage::Reset() {
    moduleFlags = BIT(ModuleBit::Standard);
    standardModule.Reset();
    shapeModule.Reset();
    customPathModule.Reset();
    forceOverLifetimeModule.Reset();
    colorOverLifetimeModule.Reset();
    speedOverLifetimeModule.Reset();
    rotationOverLifetimeModule.Reset();
    rotationBySpeedModule.Reset();
    sizeOverLifetimeModule.Reset();
    sizeBySpeedModule.Reset();
    aspectRatioOverLifetimeModule.Reset();
    trailsModule.Reset();
}

class ParticleSystemManager {
public:
    void                        Init();
    void                        Shutdown();

    ParticleSystem *            AllocParticleSystem(const char *name);
    ParticleSystem *            FindParticleSystem(const char *name) const;
    ParticleSystem *            GetParticleSystem(const char *name);

    void                        ReleaseParticleSystem(ParticleSystem *particleSystem, bool immediateDestroy = false);
    void                        DestroyParticleSystem(ParticleSystem *particleSystem);
    void                        DestroyUnusedParticleSystems();

    void                        PrecacheParticleSystem(const char *name);

    void                        RenameParticleSystem(ParticleSystem *particleSystem, const Str &newName);

    static ParticleSystem *     defaultParticleSystem;

private:
    StrIHashMap<ParticleSystem *> particleSystemHashMap;
};

extern ParticleSystemManager    particleSystemManager;

BE_NAMESPACE_END
