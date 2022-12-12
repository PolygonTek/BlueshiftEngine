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

#include "Core/Object.h"

BE_NAMESPACE_BEGIN

class PhysicsWorld;

class PhysicsSettings : public Object {
    friend class GameSettings;

public:
    OBJECT_PROTOTYPE(PhysicsSettings);

    PhysicsSettings();

    int                         GetSolver() const;
    void                        SetSolver(int solver);

    int                         GetSolverIterations() const;
    void                        SetSolverIterations(int iterationCount);

    int                         GetFrameRate() const;
    void                        SetFrameRate(int frameRate);

    float                       GetMaximumAllowedTimeStep() const;
    void                        SetMaximumAllowedTimeStep(float timeStep);

    Vec3                        GetGravity() const;
    void                        SetGravity(const Vec3 &gravity);

    float                       GetAirDensity() const;
    void                        SetAirDensity(float airDensity);

    int                         GetFilterMaskElement(int index) const;
    void                        SetFilterMaskElement(int index, int mask);
    int                         GetFilterMaskCount() const;
    void                        SetFilterMaskCount(int count);

    static PhysicsSettings *    Load(const char *filename, PhysicsWorld *physicsWorld);

private:
    PhysicsWorld *              physicsWorld;
    int                         numFilterMasks;
};

BE_NAMESPACE_END
