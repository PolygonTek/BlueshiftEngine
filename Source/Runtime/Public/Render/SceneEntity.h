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

    SceneEntity

-------------------------------------------------------------------------------
*/

#include "Core/WStr.h"
#include "Math/Math.h"
#include "Containers/Array.h"
#include "ParticleSystem.h"

BE_NAMESPACE_BEGIN

struct viewEntity_t;
struct DbvtProxy;
class Mat3x4;
class Skeleton;
class Material;
class Skin;
class Mesh;
class Font;
class ParticleSystem;
class Particle;

class SceneEntity {
    friend class RenderWorld;

public:
    enum MaterialParm {
        RedParm,
        GreenParm,
        BlueParm,
        AlphaParm,
        TimeOffsetParm,
        TimeScaleParm,
        MaxMaterialParms    // should be less than MAX_EXPR_LOCALPARMS
    };

    enum WireframeMode {
        ShowNone,
        ShowVisibleFront,
        ShowAllFront,
        ShowAllFrontAndBack
    };

    enum TextAnchor {
        UpperLeft,
        UpperCenter,
        UpperRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        LowerLeft,
        LowerCenter,
        LowerRight
    };

    enum TextAlignment {
        Left,
        Center,
        Right
    };

    struct Parms {
        Vec3                origin;
        Vec3                scale;
        Mat3                axis;
        AABB                aabb;                   // non-scaled AABB (don't use cleared AABB)
        float               maxVisDist;
        int                 layer;
        int                 time;

        Mesh *              mesh;
        const Skeleton *    skeleton;
        int                 numJoints;
        Mat3x4 *            joints;

        Font *              font;
        WStr                text;
        TextAnchor          textAnchor;
        TextAlignment       textAlignment;
        float               textScale;
        float               lineSpacing;

        ParticleSystem *    particleSystem;
        Array<Particle *>   stageParticles;
        Array<float>        stageStartDelay;
        
        Array<Material *>   materials;
        float               materialParms[MaxMaterialParms];
        Skin *              customSkin;
        WireframeMode       wireframeMode;
        Color4              wireframeColor;

        bool                firstPersonOnly;
        bool                thirdPersonOnly;
        bool                billboard;
        bool                depthHack;
        bool                useLightProbe;
        bool                castShadows;
        bool                receiveShadows;
        bool                occluder;               // for use in HOM
        bool                skipSelectionBuffer;
        bool                richText;
    };

    SceneEntity();
    ~SceneEntity();

    void                    Update(const Parms *parms);

    const AABB              GetAABB() const;

    const Mat4 &            GetModelMatrix() const { return modelMatrix; }
    
    const OBB &             GetWorldOBB() const { return worldOBB; }

    void                    DrawJoints() const;

    int                     index;
    Parms                   parms;
    bool                    firstUpdate;
    OBB                     worldOBB;
    Mat4                    modelMatrix;
    Mat4                    motionBlurModelMatrix[2];
    int                     viewCount;
    viewEntity_t *          viewEntity;
    DbvtProxy *             proxy;
    int                     numMeshSurfProxies;
    DbvtProxy *             meshSurfProxies;            // mesh surf proxy for static sub mesh
};

BE_NAMESPACE_END
