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

        // transform info
        Vec3                origin;
        Vec3                scale;
        Mat3                axis;
        AABB                localAABB;      // non-scaled local AABB (shouldn't be empty AABB)

        // static mesh or skinned mesh
        Mesh *              mesh;
        const Skeleton *    skeleton;
        int                 numJoints;
        Mat3x4 *            joints;

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

        // wire frame parameters
        WireframeMode       wireframeMode;
        Color4              wireframeColor;

        float               maxVisDist;
    };

    RenderObject();
    ~RenderObject();

    void                    Update(const State *state);

                            /// Returns non-scaled AABB in local space.
    const AABB              GetLocalAABB() const;

                            /// Returns OBB in world space.
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns local to world matrix.
    const Mat4 &            GetObjectToWorldMatrix() const { return worldMatrix; }

    int                     index;
    State                   state;
    bool                    firstUpdate;

    OBB                     worldOBB;
    Mat4                    worldMatrix;
    Mat4                    prevWorldMatrix;

    int                     viewCount;
    VisibleObject *         visibleObject;

    DbvtProxy *             proxy;
    int                     numMeshSurfProxies;
    DbvtProxy *             meshSurfProxies;            // mesh surf proxy for static sub mesh
};

BE_NAMESPACE_END
