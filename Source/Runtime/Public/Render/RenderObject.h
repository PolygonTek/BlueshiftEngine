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

    RenderObject

-------------------------------------------------------------------------------
*/

#include "Core/WStr.h"
#include "Math/Math.h"
#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

struct DbvtProxy;
class VisibleObject;
class Mat3x4;
class Skeleton;
class Material;
class Skin;
class Mesh;
class Font;
class ParticleSystem;
class Particle;

class RenderObject {
    friend class RenderWorld;

public:
    enum Flag {
        StaticFlag          = BIT(0),
        FirstPersonOnlyFlag = BIT(1),
        ThirdPersonOnlyFlag = BIT(2),
        BillboardFlag       = BIT(3),
        DepthHackFlag       = BIT(4),
        UseLightProbeFlag   = BIT(5),
        CastShadowsFlag     = BIT(6),
        ReceiveShadowsFlag  = BIT(7),
        OccluderFlag        = BIT(8),   // for use in HOM
        SkipSelectionFlag   = BIT(9),
        RichTextFlag        = BIT(10),
    };

    enum MaterialParm {
        RedParm,
        GreenParm,
        BlueParm,
        AlphaParm,
        TimeOffsetParm,
        TimeScaleParm,
        MaxMaterialParms    // should be less than MAX_EXPR_LOCALPARMS
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

    enum WireframeMode {
        ShowNone,
        ShowVisibleFront,
        ShowAllFront,
        ShowAllFrontAndBack
    };

    struct State {
        int                 flags;
        int                 layer;
        int                 time;
        float               maxVisDist;

        // transform info
        Vec3                origin;             // object position in world space
        Vec3                scale;              // object scaling
        Mat3                axis;               // object orientation
        AABB                localAABB;          // non-scaled local AABB (shouldn't be empty)

        // wire frame info
        WireframeMode       wireframeMode;
        Color4              wireframeColor;

        // static/skinned mesh
        Mesh *              mesh;               // instantiated mesh
        const Skeleton *    skeleton;           // skeleton information for skeletal mesh
        int                 numJoints;          // number of joints
        Mat3x4 *            joints;             // joint transform matrices to animate skeletal mesh

        // text rendering
        Font *              font;
        WStr                text;
        TextAnchor          textAnchor;
        TextAlignment       textAlignment;
        float               textScale;
        float               lineSpacing;

        // particle system
        ParticleSystem *    particleSystem;
        Array<Particle *>   stageParticles;
        Array<float>        stageStartDelay;
        
        // materials
        Array<Material *>   materials;
        float               materialParms[MaxMaterialParms];
        Skin *              customSkin;
    };

    RenderObject();
    ~RenderObject();

    void                    Update(const State *state);

                            /// Returns non-scaled AABB in local space.
    const AABB              GetLocalAABB() const;

                            /// Returns OBB in world space.
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns local to world matrix.
    const Mat3x4 &          GetObjectToWorldMatrix() const { return worldMatrix; }

    int                     index;
    bool                    firstUpdate;

    State                   state;

    OBB                     worldOBB;
    Mat3x4                  worldMatrix;
    Mat3x4                  prevWorldMatrix;

    VisibleObject *         visObject;
    int                     viewCount;

    DbvtProxy *             proxy;
    int                     numMeshSurfProxies;
    DbvtProxy *             meshSurfProxies;            // mesh surf proxy for static sub mesh
};

BE_NAMESPACE_END
