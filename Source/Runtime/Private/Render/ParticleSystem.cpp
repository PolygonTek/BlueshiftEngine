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
#include "Asset/GuidMapper.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

#define PRTS_VERSION 1

static const char *moduleNames[] = {
    "Standard",
    "Shape",
    "CustomPath",
    "LTColor",
    "LTSpeed",
    "LTForce",
    "LTRotation",
    "RotationBySpeed",
    "LTSize",
    "SizeBySpeed",
    "LTAspectRatio",
    "Trails"
};

static const char *simulationSpaceNames[] = {
    "Local",
    "Global"
};

static const char *orientationNames[] = {
    "View",
    "ViewZ",
    "Aimed",
    "AimedZ",
    "X",
    "Y",
    "Z"
};

static const char *minMaxVarTypeNames[] = {
    "Constant",
    "RandomBetweenTwoConstants"
};

static const char *timedVarTypeNames[] = {
    "Constant",
    "Curve"
};

static const char *timedMinMaxVarTypeNames[] = {
    "Constant",
    "Curve",
    "RandomBetweenTwoConstants",
    "RandomBetweenTwoCurves"
};

static const char *timeWrapModeNames[] = {
    "Clamp",
    "Loop",
    "PingPong"
};

static const char *shapeNames[] = {
    "Box",
    "Sphere",
    "Circle",
    "Cone"
};

static const char *customPathNames[] = {
    "Cone",
    "Spherical",
    "Helix"
};

void ParticleSystem::Purge() {
    for (int stageIndex = 0; stageIndex < stages.Count(); stageIndex++) {
        materialManager.ReleaseMaterial(stages[stageIndex].standardModule.material);
    }

    stages.Clear();
}

bool ParticleSystem::Create(const char *text) {
    Str token;

    Purge();

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName.c_str());

    lexer.ExpectTokenString("numStages");
    int numStages = lexer.ParseInt();
    stages.SetCount(numStages);

    for (int stageIndex = 0; stageIndex < numStages; stageIndex++) {
        if (!ParseStage(lexer, stages[stageIndex])) {
            return false;
        }
    }

    return true;
}

bool ParticleSystem::ParseStage(Lexer &lexer, Stage &stage) const {
    Str token;

    stage.Reset();
    stage.skipRender = false;

    lexer.ExpectTokenString("stage");

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("name")) {
            Str stageName;
            if (lexer.ReadToken(&stageName)) {
                stage.name = stageName;
            } else {
                BE_WARNLOG(L"missing stage name in particleSystem '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("module")) {
            Str moduleName;
            if (lexer.ReadToken(&moduleName)) {
                if (moduleName == moduleNames[StandardModuleBit]) {
                    stage.moduleFlags |= BIT(StandardModuleBit);

                    if (!ParseStandardModule(lexer, stage.standardModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[ShapeModuleBit]) {
                    stage.moduleFlags |= BIT(ShapeModuleBit);

                    if (!ParseShapeModule(lexer, stage.shapeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[CustomPathModuleBit]) {
                    stage.moduleFlags |= BIT(CustomPathModuleBit);

                    if (!ParseCustomPathModule(lexer, stage.customPathModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTColorModuleBit]) {
                    stage.moduleFlags |= BIT(LTColorModuleBit);

                    if (!ParseLTColorModule(lexer, stage.colorOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTSpeedModuleBit]) {
                    stage.moduleFlags |= BIT(LTSpeedModuleBit);

                    if (!ParseLTSpeedModule(lexer, stage.speedOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTForceModuleBit]) {
                    stage.moduleFlags |= BIT(LTForceModuleBit);

                    if (!ParseLTForceModule(lexer, stage.forceOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTRotationModuleBit]) {
                    stage.moduleFlags |= BIT(LTRotationModuleBit);

                    if (!ParseLTRotationModule(lexer, stage.rotationOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[RotationBySpeedModuleBit]) {
                    stage.moduleFlags |= BIT(RotationBySpeedModuleBit);

                    if (!ParseRotationBySpeedModule(lexer, stage.rotationBySpeedModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTSizeModuleBit]) {
                    stage.moduleFlags |= BIT(LTSizeModuleBit);

                    if (!ParseLTSizeModule(lexer, stage.sizeOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[SizeBySpeedModuleBit]) {
                    stage.moduleFlags |= BIT(SizeBySpeedModuleBit);

                    if (!ParseSizeBySpeedModule(lexer, stage.sizeBySpeedModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[LTAspectRatioModuleBit]) {
                    stage.moduleFlags |= BIT(LTAspectRatioModuleBit);

                    if (!ParseLTAspectRatioModule(lexer, stage.aspectRatioOverLifetimeModule)) {
                        return false;
                    }
                } else if (moduleName == moduleNames[TrailsModuleBit]) {
                    stage.moduleFlags |= BIT(TrailsModuleBit);

                    if (!ParseTrailsModule(lexer, stage.trailsModule)) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                BE_WARNLOG(L"missing module name in particleSystem '%hs'\n", hashName.c_str());
            }
        } else {
            BE_WARNLOG(L"unknown particleSystem stage parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseStandardModule(Lexer &lexer, StandardModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("count")) {
            module.count = lexer.ParseInt();
        } else if (!token.Icmp("spawnBunching")) {
            module.spawnBunching = lexer.ParseFloat();
        } else if (!token.Icmp("lifeTime")) {
            module.lifeTime = lexer.ParseFloat();
        } else if (!token.Icmp("deadTime")) {
            module.deadTime = lexer.ParseFloat();
        } else if (!token.Icmp("looping")) {
            if (lexer.ReadToken(&token)) {
                module.looping = (token == "false" ? false : true); 
            } else {
                BE_WARNLOG(L"missing boolean value for the looping param in particleSystem '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("prewarm")) {
            if (lexer.ReadToken(&token, false)) {
                module.prewarm = (token == "false" ? false : true);
            } else {
                BE_WARNLOG(L"missing boolean value for the prewarm param in particleSystem '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("maxCycles")) {
            module.maxCycles = lexer.ParseInt();
        } else if (!token.Icmp("simulationSpeed")) {
            module.simulationSpeed = lexer.ParseFloat();
        } else if (!token.Icmp("simulationSpace")) {
            ParseSimulationSpace(lexer, &module.simulationSpace);
        } else if (!token.Icmp("orientation")) {
            ParseOrientation(lexer, &module.orientation);
        } else if (!token.Icmp("material")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid materialGuid = Guid::FromString(token);
                const Str materialPath = resourceGuidMapper.Get(materialGuid);
                module.material = materialManager.GetMaterial(materialPath);
            } else {
                BE_WARNLOG(L"missing material name in particleSystem '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("animation")) {
            if (lexer.ReadToken(&token)) {
                module.animation = (token == "false" ? false : true);
            } else {
                BE_WARNLOG(L"missing boolean value for the animation param in particleSystem '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("animFrames")) {
            module.animFrames[0] = lexer.ParseInt();
            module.animFrames[1] = lexer.ParseInt();
        } else if (!token.Icmp("animFps")) {
            module.animFps = lexer.ParseInt();
        } else if (!token.Icmp("startDelay")) {
            ParseMinMaxCurve(lexer, &module.startDelay);
        } else if (!token.Icmp("startColor")) {
            lexer.ParseVec(4, module.startColor);
        } else if (!token.Icmp("startSpeed")) {
            ParseMinMaxCurve(lexer, &module.startSpeed);
        } else if (!token.Icmp("startSize")) {
            ParseMinMaxCurve(lexer, &module.startSize);
        } else if (!token.Icmp("startAspectRatio")) {
            ParseMinMaxCurve(lexer, &module.startAspectRatio);
        } else if (!token.Icmp("startRotation")) {
            ParseMinMaxCurve(lexer, &module.startRotation);
        } else if (!token.Icmp("randomizeRotation")) {
            module.randomizeRotation = lexer.ParseFloat();
        } else if (!token.Icmp("gravity")) {
            module.gravity = lexer.ParseFloat();
        } else {
            BE_WARNLOG(L"unknown standard module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseSimulationSpace(Lexer &lexer, StandardModule::SimulationSpace *simulationSpace) const {
    Str token;
    
    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(simulationSpaceNames); i++) {
            if (!token.Icmp(simulationSpaceNames[i])) {
                *simulationSpace = (StandardModule::SimulationSpace)i;
                return true;
            }
        }

        BE_WARNLOG(L"unknown orientation '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    BE_WARNLOG(L"missing orientation name in particleSystem '%hs'\n", hashName.c_str());
    return false;
}

bool ParticleSystem::ParseOrientation(Lexer &lexer, StandardModule::Orientation *orientation) const {
    Str token;

    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(orientationNames); i++) {
            if (!token.Icmp(orientationNames[i])) {
                *orientation = (StandardModule::Orientation)i;
                return true;
            }
        }

        BE_WARNLOG(L"unknown orientation '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    BE_WARNLOG(L"missing orientation name in particleSystem '%hs'\n", hashName.c_str());
    return false;
}

bool ParticleSystem::ParseMinMaxCurve(Lexer &lexer, MinMaxCurve *var) const {
    Str token;

    var->maxCurve.Clear();
    var->minCurve.Clear();

    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(timedMinMaxVarTypeNames); i++) {
            if (!token.Icmp(timedMinMaxVarTypeNames[i])) {
                var->type = (MinMaxCurve::Type)i;

                if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
                    return false;
                }

                lexer.ExpectTokenString("scalar");
                var->scalar = lexer.ParseFloat();

                lexer.ExpectTokenString("wrapMode");

                Hermite<float>::TimeWrapMode minTimeWrapMode;
                ParseTimeWrapMode(lexer, &minTimeWrapMode);
                var->minCurve.SetMinTimeWrapMode(minTimeWrapMode);
                var->maxCurve.SetMinTimeWrapMode(minTimeWrapMode);

                Hermite<float>::TimeWrapMode maxTimeWrapMode;
                ParseTimeWrapMode(lexer, &maxTimeWrapMode);
                var->minCurve.SetMaxTimeWrapMode(maxTimeWrapMode);
                var->maxCurve.SetMaxTimeWrapMode(maxTimeWrapMode);

                for (int curveIndex = 0; curveIndex < 2; curveIndex++) {
                    auto &curve = curveIndex == 0 ? var->minCurve : var->maxCurve;

                    lexer.ExpectTokenString(curveIndex == 0 ? "minCurve" : "maxCurve");
                    lexer.ExpectTokenString("{");

                    lexer.ExpectTokenString("numKeys");
                    int numKeys = lexer.ParseInt();

                    lexer.ExpectTokenString("keys");
                    lexer.ExpectTokenString("{");

                    for (int keyIndex = 0; keyIndex < numKeys; keyIndex++) {
                        float time = lexer.ParseFloat();
                        float point = lexer.ParseFloat();
                        float outgoingSlope = lexer.ParseFloat();
                        float incomingSlope = lexer.ParseFloat();
                        int tangentModeFlags = lexer.ParseInt();

                        curve.AddPoint(time, point);
                        curve.SetOutgoingSlope(keyIndex, outgoingSlope);
                        curve.SetIncomingSlope(keyIndex, incomingSlope);
                        curve.SetTangentModeFlags(keyIndex, tangentModeFlags);
                    }

                    lexer.ExpectTokenString("}");

                    lexer.ExpectTokenString("}");
                }

                if (!lexer.ExpectPunctuation(P_BRACECLOSE)) {
                    return false;
                }
                return true;
            }
        }

        BE_WARNLOG(L"unknown MinMaxCurve type '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    return false;
}

bool ParticleSystem::ParseTimeWrapMode(Lexer &lexer, Hermite<float>::TimeWrapMode *timeWrapMode) const {
    Str token;

    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(timeWrapModeNames); i++) {
            if (!token.Icmp(timeWrapModeNames[i])) {
                *timeWrapMode = (Hermite<float>::TimeWrapMode)i;
                return true;
            }
        }

        BE_WARNLOG(L"unknown time wrap mode '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    BE_WARNLOG(L"missing time wrap mode name in particleSystem '%hs'\n", hashName.c_str());
    return false;
}

bool ParticleSystem::ParseShapeModule(Lexer &lexer, ShapeModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("shape")) {
            ParseShape(lexer, &module.shape);
        } else if (!token.Icmp("extents")) {
            lexer.ParseVec(3, module.extents);
        } else if (!token.Icmp("radius")) {
            module.radius = lexer.ParseFloat();
        } else if (!token.Icmp("thickness")) {
            module.thickness = lexer.ParseFloat();
        } else if (!token.Icmp("angle")) {
            module.angle = lexer.ParseFloat();
        } else if (!token.Icmp("randomizeDir")) {
            module.randomizeDir = lexer.ParseFloat();
        } else {
            BE_WARNLOG(L"unknown Shape module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseShape(Lexer &lexer, ShapeModule::Shape *shape) const {
    Str token;

    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(shapeNames); i++) {
            if (!token.Icmp(shapeNames[i])) {
                *shape = (ShapeModule::Shape)i;
                return true;
            }
        }

        BE_WARNLOG(L"unknown shape '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    BE_WARNLOG(L"missing shape name in particleSystem '%hs'\n", hashName.c_str());
    return false;
}

bool ParticleSystem::ParseCustomPathModule(Lexer &lexer, CustomPathModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("customPath")) {
            ParseCustomPath(lexer, &module.customPath);
        } else if (!token.Icmp("radialSpeed")) {
            module.radialSpeed = lexer.ParseFloat();
        } else if (!token.Icmp("axialSpeed")) {
            module.axialSpeed = lexer.ParseFloat();
        } else if (!token.Icmp("innerRadius")) {
            module.innerRadius = lexer.ParseFloat();
        } else if (!token.Icmp("outerRadius")) {
            module.outerRadius = lexer.ParseFloat();
        } else {
            BE_WARNLOG(L"unknown CustomPath module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseCustomPath(Lexer &lexer, CustomPathModule::CustomPath *customPath) const {
    Str token;

    if (lexer.ReadToken(&token, false)) {
        for (int i = 0; i < COUNT_OF(customPathNames); i++) {
            if (!token.Icmp(customPathNames[i])) {
                *customPath = (CustomPathModule::CustomPath)i;
                return true;
            }
        }

        BE_WARNLOG(L"unknown customPath '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    BE_WARNLOG(L"missing customPath name in particleSystem '%hs'\n", hashName.c_str());
    return false;
}

bool ParticleSystem::ParseLTForceModule(Lexer &lexer, LTForceModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("forceX")) {
            ParseMinMaxCurve(lexer, &module.force[0]);
        } else if (!token.Icmp("forceY")) {
            ParseMinMaxCurve(lexer, &module.force[1]);
        } else if (!token.Icmp("forceZ")) {
            ParseMinMaxCurve(lexer, &module.force[2]);
        } else {
            BE_WARNLOG(L"unknown LTForce module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseLTColorModule(Lexer &lexer, LTColorModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("targetColor")) {
            lexer.ParseVec(4, module.targetColor);
        } else if (!token.Icmp("fadeLocation")) {
            module.fadeLocation = lexer.ParseFloat();
        } else {
            BE_WARNLOG(L"unknown LTColor module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseLTSpeedModule(Lexer &lexer, LTSpeedModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("speed")) {
            ParseMinMaxCurve(lexer, &module.speed);
        } else {
            BE_WARNLOG(L"unknown LTSpeed module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseLTRotationModule(Lexer &lexer, LTRotationModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("rotation")) {
            ParseMinMaxCurve(lexer, &module.rotation);
        } else {
            BE_WARNLOG(L"unknown LTRotation module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseRotationBySpeedModule(Lexer &lexer, RotationBySpeedModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("rotation")) {
            ParseMinMaxCurve(lexer, &module.rotation);
        } else if (!token.Icmp("speedRange")) {
            lexer.ParseVec(2, module.speedRange);
        } else {
            BE_WARNLOG(L"unknown RotationBySpeed module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseLTSizeModule(Lexer &lexer, LTSizeModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("size")) {
            ParseMinMaxCurve(lexer, &module.size);
        } else {
            BE_WARNLOG(L"unknown LTSize module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseSizeBySpeedModule(Lexer &lexer, SizeBySpeedModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("size")) {
            ParseMinMaxCurve(lexer, &module.size);
        } else if (!token.Icmp("speedRange")) {
            lexer.ParseVec(2, module.speedRange);
        } else {
            BE_WARNLOG(L"unknown SizeBySpeed module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseLTAspectRatioModule(Lexer &lexer, LTAspectRatioModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("aspectRatio")) {
            ParseMinMaxCurve(lexer, &module.aspectRatio);
        } else {
            BE_WARNLOG(L"unknown LTAspectRatio module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

bool ParticleSystem::ParseTrailsModule(Lexer &lexer, TrailsModule &module) const {
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("count")) {
            module.count = lexer.ParseInt();
        } else if (!token.Icmp("length")) {
            module.length = lexer.ParseFloat();
        } else if (!token.Icmp("trailScale")) {
            module.trailScale = lexer.ParseFloat();
        } else if (!token.Icmp("trailCut")) {
            if (lexer.ReadToken(&token)) {
                module.trailCut = (token == "false" ? false : true);
            } else {
                BE_WARNLOG(L"missing boolean value for the trailCut param in particleSystem '%hs'\n", hashName.c_str());
            }
        } else {
            BE_WARNLOG(L"unknown Trails module parameter '%hs' in particleSystem '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    return true;
}

void ParticleSystem::CreateDefaultParticleSystem() {
    Purge();

    AddStage();
}

void ParticleSystem::AddStage() {
    Stage &stage = stages.Alloc();
    stage.name.sPrintf("stage %i", stages.Count() - 1);
    stage.skipRender = false;
    stage.Reset();

    stage.moduleFlags |= BIT(ShapeModuleBit);
}

bool ParticleSystem::RemoveStage(int stageIndex) {
    if (stageIndex < 0 || stageIndex >= stages.Count() ) {
        BE_WARNLOG(L"ParticleSystem::RemoveStage: out of index %i\n", stageIndex);
        return false;
    }

    materialManager.ReleaseMaterial(stages[stageIndex].standardModule.material);

    return stages.RemoveIndex(stageIndex);
}

void ParticleSystem::SwapStages(int stageIndex0, int stageIndex1) {
    if (stageIndex0 < 0 || stageIndex0 >= stages.Count() ||
        stageIndex1 < 0 || stageIndex1 >= stages.Count()) {
        BE_WARNLOG(L"ParticleSystem::SwapStages: out of index %i, %i\n", stageIndex0, stageIndex1);
        return;
    }

    Swap(stages[stageIndex0], stages[stageIndex1]);
}

bool ParticleSystem::Load(const char *filename) {
    Purge();

    Str prtSysFilename = filename;
    if (!Str::CheckExtension(filename, ".prts")) {
        prtSysFilename.SetFileExtension(".prts");
    }

    BE_LOG(L"Loading particle system '%hs'...\n", filename);

    char *data;
    int size = (int)fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(data, size, hashName);

    if (!lexer.ExpectTokenString("particleSystem")) {
        fileSystem.FreeFile(data);
        return false;
    }

    int version = lexer.ParseInt();
    if (version != PRTS_VERSION) {
        lexer.Error("ParticleSystem::Load: Invalid version %d\n", version);
        fileSystem.FreeFile(data);
        return false;
    }

    Create(data + lexer.GetCurrentOffset());

    fileSystem.FreeFile(data);

    return true;
}

static void WriteMinMaxCurve(File *fp, const Str &name, const MinMaxCurve &var, Str &indentSpace) {
    fp->Printf("%s%s \"%s\" {\n", indentSpace.c_str(), name.c_str(), timedMinMaxVarTypeNames[var.type]);
    indentSpace += "  ";

    fp->Printf("%sscalar %.3f\n", indentSpace.c_str(), var.scalar);
       
    fp->Printf("%swrapMode \"%s\" \"%s\"\n", indentSpace.c_str(),
        timeWrapModeNames[var.maxCurve.GetMinTimeWrapMode()], 
        timeWrapModeNames[var.maxCurve.GetMaxTimeWrapMode()]);

    for (int curveIndex = 0; curveIndex < 2; curveIndex++) {
        const auto &curve = curveIndex == 0 ? var.minCurve : var.maxCurve;

        fp->Printf("%s%s {\n", indentSpace.c_str(), curveIndex == 0 ? "minCurve" : "maxCurve");
        indentSpace += "  ";

        fp->Printf("%snumKeys %i\n", indentSpace.c_str(), curve.NumKeys());
        fp->Printf("%skeys {\n", indentSpace.c_str());
        indentSpace += "  ";

        for (int keyIndex = 0; keyIndex < curve.NumKeys(); keyIndex++) {
            fp->Printf("%s%.3f %.3f %.3f %.3f %i\n", indentSpace.c_str(),
                curve.GetTime(keyIndex),
                curve.GetPoint(keyIndex),
                curve.GetOutgoingSlope(keyIndex),
                curve.GetIncomingSlope(keyIndex),
                curve.GetTangentModeFlags(keyIndex));
        }

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());
    }

    indentSpace.Truncate(indentSpace.Length() - 2);
    fp->Printf("%s}\n", indentSpace.c_str());
}

void ParticleSystem::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"ParticleSystem::Write: file open error\n");
        return;
    }

    Str indentSpace;

    fp->Printf("particleSystem %i\n", PRTS_VERSION);
    fp->Printf("%snumStages %i\n", indentSpace.c_str(), stages.Count());

    for (int stageIndex = 0; stageIndex < stages.Count(); stageIndex++) {
        const Stage &stage = stages[stageIndex];

        fp->Printf("%sstage {\n", indentSpace.c_str());
        indentSpace += "  ";

        // Stage name
        fp->Printf("%sname \"%s\"\n", indentSpace.c_str(), stage.name.c_str());

        // StandardModule is always included
        const StandardModule &standardModule = stage.standardModule;

        fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[0]);
        indentSpace += "  ";

        fp->Printf("%scount %i\n", indentSpace.c_str(), standardModule.count);
        fp->Printf("%sspawnBunching %.3f\n", indentSpace.c_str(), standardModule.spawnBunching);
        fp->Printf("%slifeTime %.3f\n", indentSpace.c_str(), standardModule.lifeTime);
        fp->Printf("%sdeadTime %.3f\n", indentSpace.c_str(), standardModule.deadTime);
        fp->Printf("%slooping %s\n", indentSpace.c_str(), standardModule.looping ? "true" : "false");
        fp->Printf("%sprewarm %s\n", indentSpace.c_str(), standardModule.prewarm ? "true" : "false");
        fp->Printf("%smaxCycles %i\n", indentSpace.c_str(), standardModule.maxCycles);
        
        fp->Printf("%ssimulationSpace \"%s\"\n", indentSpace.c_str(), simulationSpaceNames[standardModule.simulationSpace]);
        fp->Printf("%ssimulationSpeed %.3f\n", indentSpace.c_str(), standardModule.simulationSpeed);

        const Guid materialGuid = resourceGuidMapper.Get(standardModule.material->GetHashName());
        fp->Printf("%smaterial \"%s\"\n", indentSpace.c_str(), materialGuid.ToString());
        fp->Printf("%sanimation %s\n", indentSpace.c_str(), standardModule.animation ? "true" : "false");
        fp->Printf("%sanimFrames %i %i\n", indentSpace.c_str(), standardModule.animFrames[0], standardModule.animFrames[1]);
        fp->Printf("%sanimFps %i\n", indentSpace.c_str(), standardModule.animFps);
        fp->Printf("%sorientation \"%s\"\n", indentSpace.c_str(), orientationNames[standardModule.orientation]);

        WriteMinMaxCurve(fp, "startDelay", standardModule.startDelay, indentSpace);

        fp->Printf("%sstartColor ( %s )\n", indentSpace.c_str(), standardModule.startColor.ToString());

        WriteMinMaxCurve(fp, "startSpeed", standardModule.startSpeed, indentSpace);
        WriteMinMaxCurve(fp, "startSize", standardModule.startSize, indentSpace);
        WriteMinMaxCurve(fp, "startAspectRatio", standardModule.startAspectRatio, indentSpace);
        WriteMinMaxCurve(fp, "startRotation", standardModule.startRotation, indentSpace);

        fp->Printf("%srandomizeRotation %.3f\n", indentSpace.c_str(), standardModule.randomizeRotation);
        fp->Printf("%sgravity %.3f\n", indentSpace.c_str(), standardModule.gravity);

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());

        // ShapeModule
        if (stage.moduleFlags & BIT(ShapeModuleBit)) {
            const ShapeModule &shapeModule = stage.shapeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[ShapeModuleBit]);
            indentSpace += "  ";

            fp->Printf("%sshape \"%s\"\n", indentSpace.c_str(), shapeNames[shapeModule.shape]);

            fp->Printf("%sextents ( %s )\n", indentSpace.c_str(), shapeModule.extents.ToString());
            fp->Printf("%sradius %.3f\n", indentSpace.c_str(), shapeModule.radius);
            fp->Printf("%sthickness %.3f\n", indentSpace.c_str(), shapeModule.thickness);
            fp->Printf("%sangle %.3f\n", indentSpace.c_str(), shapeModule.angle);
            fp->Printf("%srandomizeDir %.3f\n", indentSpace.c_str(), shapeModule.randomizeDir);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // CustomPathModule
        if (stage.moduleFlags & BIT(CustomPathModuleBit)) {
            const CustomPathModule &customPathModule = stage.customPathModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[CustomPathModuleBit]);
            indentSpace += "  ";

            fp->Printf("%scustomPath \"%s\"\n", indentSpace.c_str(), customPathNames[customPathModule.customPath]);

            fp->Printf("%sradialSpeed %.3f\n", indentSpace.c_str(), customPathModule.radialSpeed);
            fp->Printf("%saxialSpeed %.3f\n", indentSpace.c_str(), customPathModule.axialSpeed);
            fp->Printf("%sinnerRadius %.3f\n", indentSpace.c_str(), customPathModule.innerRadius);
            fp->Printf("%souterRadius %.3f\n", indentSpace.c_str(), customPathModule.outerRadius);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }        

        // LTColorModule
        if (stage.moduleFlags & BIT(LTColorModuleBit)) {
            const LTColorModule &colorOverLifetimeModule = stage.colorOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTColorModuleBit]);
            indentSpace += "  ";

            fp->Printf("%stargetColor ( %s )\n", indentSpace.c_str(), colorOverLifetimeModule.targetColor.ToString());
            fp->Printf("%sfadeLocation %.3f\n", indentSpace.c_str(), colorOverLifetimeModule.fadeLocation);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // LTSpeedModule
        if (stage.moduleFlags & BIT(LTSpeedModuleBit)) {
            const LTSpeedModule &speedOverLifetimeModule = stage.speedOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTSpeedModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "speed", speedOverLifetimeModule.speed, indentSpace);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // LTForceModule
        if (stage.moduleFlags & BIT(LTForceModuleBit)) {
            const LTForceModule &forceOverLifetimeModule = stage.forceOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTForceModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "forceX", forceOverLifetimeModule.force[0], indentSpace);
            WriteMinMaxCurve(fp, "forceY", forceOverLifetimeModule.force[1], indentSpace);
            WriteMinMaxCurve(fp, "forceZ", forceOverLifetimeModule.force[2], indentSpace);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // LTRotationModule
        if (stage.moduleFlags & BIT(LTRotationModuleBit)) {
            const LTRotationModule &rotationOverLifetimeModule = stage.rotationOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTRotationModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "rotation", rotationOverLifetimeModule.rotation, indentSpace);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // RotationBySpeedModule
        if (stage.moduleFlags & BIT(RotationBySpeedModuleBit)) {
            const RotationBySpeedModule &rotationBySpeedModule = stage.rotationBySpeedModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[RotationBySpeedModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "rotation", rotationBySpeedModule.rotation, indentSpace);
            fp->Printf("%sspeedRange ( %s )\n", indentSpace.c_str(), rotationBySpeedModule.speedRange.ToString());

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // LTSizeModule
        if (stage.moduleFlags & BIT(LTSizeModuleBit)) {
            const LTSizeModule &sizeOverLifetimeModule = stage.sizeOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTSizeModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "size", sizeOverLifetimeModule.size, indentSpace);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // SizeBySpeedModule
        if (stage.moduleFlags & BIT(SizeBySpeedModuleBit)) {
            const SizeBySpeedModule &sizeBySpeedModule = stage.sizeBySpeedModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[SizeBySpeedModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "size", sizeBySpeedModule.size, indentSpace);
            fp->Printf("%sspeedRange ( %s )\n", indentSpace.c_str(), sizeBySpeedModule.speedRange.ToString());

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // LTAspectRatioModule
        if (stage.moduleFlags & BIT(LTAspectRatioModuleBit)) {
            const LTAspectRatioModule &aspectRatioOverLifetimeModule = stage.aspectRatioOverLifetimeModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[LTAspectRatioModuleBit]);
            indentSpace += "  ";

            WriteMinMaxCurve(fp, "aspectRatio", aspectRatioOverLifetimeModule.aspectRatio, indentSpace);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        // TrailsModule
        if (stage.moduleFlags & BIT(TrailsModuleBit)) {
            const TrailsModule &trailsModule = stage.trailsModule;

            fp->Printf("%smodule \"%s\" {\n", indentSpace.c_str(), moduleNames[TrailsModuleBit]);
            indentSpace += "  ";

            fp->Printf("%scount %i\n", indentSpace.c_str(), trailsModule.count);
            fp->Printf("%slength %.3f\n", indentSpace.c_str(), trailsModule.length);
            fp->Printf("%strailScale %.3f\n", indentSpace.c_str(), trailsModule.trailScale);
            fp->Printf("%strailCut %s\n", indentSpace.c_str(), trailsModule.trailCut ? "true" : "false");

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());
    }

    fileSystem.CloseFile(fp);
}

bool ParticleSystem::Reload() {
    Str _hashName = hashName;
    bool ret = Load(_hashName);
    return ret;
}

BE_NAMESPACE_END
