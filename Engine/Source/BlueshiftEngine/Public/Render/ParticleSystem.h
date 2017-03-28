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

#include "Material.h"

BE_NAMESPACE_BEGIN

struct ParticleSystem {
    struct StandardModule {
        int             count;
        float           depthHack;
        float           cullDistance;
        float           duration;
        float           bunching;
        Material *      material;
        Color4          startColor;
        bool            useLocalSimulationSpace;
    };

    struct ShapeModule {
        enum Shape {
            BoxShape,
            SphereShape,
            CylinderShape
        };
        Shape           shape;
        Vec4            value;
        Vec3            offset;
    };

    StandardModule      standardModule;
    ShapeModule         distributionModule;
};

BE_NAMESPACE_END
