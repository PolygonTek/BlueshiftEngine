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
#include "IO/FileSystem.h"
#include "Game/PhysicsSettings.h"
#include "Physics/Physics.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Physics Settings", PhysicsSettings, Object)
BEGIN_EVENTS(PhysicsSettings)
END_EVENTS

void PhysicsSettings::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("frameRate", "Frame Rate", int, GetFrameRate, SetFrameRate, 50, 
        "", PropertyInfo::Flag::Editor).SetRange(1, 120, 1);
    REGISTER_ACCESSOR_PROPERTY("maximumAllowedTimeStep", "Maximum Allowed Timestep", float, GetMaximumAllowedTimeStep, SetMaximumAllowedTimeStep, 1.0f/5.0f, 
        "", PropertyInfo::Flag::Editor).SetRange(1.0f/120, 1.0f, 1.0f/120);
    REGISTER_ACCESSOR_PROPERTY("solver", "Solver", int, GetSolver, SetSolver, 0, 
        "", PropertyInfo::Flag::Editor).SetEnumString("Sequential Impulse;NNCG Solver;MLCP (PGS);MLCP (Dantzig);MLCP (Lemke)");
    REGISTER_ACCESSOR_PROPERTY("solverIterations", "solverIterations", int, GetSolverIterations, SetSolverIterations, 10, 
        "", PropertyInfo::Flag::Editor).SetRange(1, 100, 1);
    REGISTER_MIXED_ACCESSOR_PROPERTY("gravity", "Gravity", Vec3, GetGravity, SetGravity, Vec3(0, 0, -9.8),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_ARRAY_PROPERTY("filterMasks", "Filter Mask", int, GetFilterMaskElement, SetFilterMaskElement, GetFilterMaskCount, SetFilterMaskCount, -1, 
        "", PropertyInfo::Flag::Editor);
}

PhysicsSettings::PhysicsSettings() {
    numFilterMasks = 0;
}

int PhysicsSettings::GetSolver() const {
    return (int)physicsWorld->GetConstraintSolver();
}

void PhysicsSettings::SetSolver(int solver) {
    physicsWorld->SetConstraintSolver((PhysicsWorld::ConstraintSolver::Enum)solver);
}

int PhysicsSettings::GetSolverIterations() const {
    return physicsWorld->GetConstraintSolverIterations();
}

void PhysicsSettings::SetSolverIterations(int iterations) {
    physicsWorld->SetConstraintSolverIterations(iterations);
}

int PhysicsSettings::GetFrameRate() const {
    return physicsWorld->GetFrameRate();
}

void PhysicsSettings::SetFrameRate(int frameRate) {
    physicsWorld->SetFrameRate(frameRate);
}

float PhysicsSettings::GetMaximumAllowedTimeStep() const {
    return physicsWorld->GetMaximumAllowedTimeStep();
}

void PhysicsSettings::SetMaximumAllowedTimeStep(float timeStep) {
    physicsWorld->SetMaximumAllowedTimeStep(timeStep);
}

Vec3 PhysicsSettings::GetGravity() const {
    return physicsWorld->GetGravity();
}

void PhysicsSettings::SetGravity(const Vec3 &gravity) {
    physicsWorld->SetGravity(gravity);
}

int PhysicsSettings::GetFilterMaskElement(int index) const {
    return physicsWorld->GetCollisionFilterMask(index);
}

void PhysicsSettings::SetFilterMaskElement(int index, int mask) {
    physicsWorld->SetCollisionFilterMask(index, mask);
}

int PhysicsSettings::GetFilterMaskCount() const {
    return numFilterMasks;
}

void PhysicsSettings::SetFilterMaskCount(int count) {
    int oldCount = numFilterMasks;

    numFilterMasks = count;

    if (count > oldCount) {
        for (int index = oldCount; index < count; index++) {
            physicsWorld->SetCollisionFilterMask(index, -1);
        }
    }
}

PhysicsSettings *PhysicsSettings::Load(const char *filename, PhysicsWorld *physicsWorld) {
    Json::Value jsonNode;
    Json::Reader jsonReader;
    bool failedToParse = false;

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (text) {
        if (!jsonReader.parse(text, jsonNode)) {
            BE_WARNLOG("Failed to parse JSON text '%s'\n", filename);
            failedToParse = true;
        }

        fileSystem.FreeFile(text);
    } else {
        failedToParse = true;
    }

    if (failedToParse) {
        jsonNode["classname"] = PhysicsSettings::metaObject.ClassName();
        jsonNode["guid"] = Guid::CreateGuid().ToString();
    }

    const char *classname = jsonNode["classname"].asCString();

    if (Str::Cmp(classname, PhysicsSettings::metaObject.ClassName())) {
        BE_WARNLOG("Unknown classname '%s'\n", classname);
        return nullptr;
    }

    const Guid guid = Guid::FromString(jsonNode["guid"].asCString());

    PhysicsSettings *physicsSettings = (PhysicsSettings *)PhysicsSettings::CreateInstance(guid);
    physicsSettings->physicsWorld = physicsWorld;
    physicsSettings->Deserialize(jsonNode);

    return physicsSettings;
}

BE_NAMESPACE_END
