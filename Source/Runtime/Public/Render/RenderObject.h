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

#include "Core/Str.h"
#include "Math/Math.h"
#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

struct DbvtProxy;
class VisObject;
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
        UseEnvProbeLightingFlag = BIT(5),
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
        MaxMaterialParms                // should be less than MAX_EXPR_LOCALPARMS
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
        int                 flags = 0;
        int                 layer = 0;
        int                 staticMask = 0;
        int                 time = 0;
        float               maxVisDist = MeterToUnit(100);

        //
        // Transform info
        //
        Vec3                origin = Vec3::origin;      ///< Object position in world space
        Vec3                scale = Vec3::one;          ///< Object scaling
        Mat3                axis = Mat3::identity;      ///< Object orientation
        AABB                localAABB = AABB::zero;     ///< Non-scaled local AABB (shouldn't be empty)

        //
        // Wireframe info
        //
        WireframeMode       wireframeMode = ShowNone;
        Color4              wireframeColor = Color4::white;

        //
        // static/skinned mesh
        //
        Mesh *              mesh = nullptr;             ///< Instantiated mesh
        const Skeleton *    skeleton = nullptr;         ///< Skeleton information for skeletal mesh
        int                 numJoints = 0;              ///< Number of joints
        Mat3x4 *            joints = nullptr;           ///< Joint transform matrices to animate skeletal mesh

        //
        // Text rendering
        //
        Font *              font = nullptr;
        Str                 text;                       ///< UTF8 encoded string
        TextAnchor          textAnchor = UpperLeft;
        TextAlignment       textAlignment = Left;
        float               textScale = 1.0f;
        float               lineSpacing = 1.0f;

        //
        // Particle system
        //
        ParticleSystem *    particleSystem = nullptr;
        Array<Particle *>   stageParticles;
        Array<float>        stageStartDelay;
        
        //
        // Materials
        //
        Array<Material *>   materials;
        float               materialParms[MaxMaterialParms] = { 1, 1, 1, 1, 0, 1 };
        Skin *              customSkin = nullptr;
    };

    RenderObject(RenderWorld *renderWorld, int index);
    ~RenderObject();

                            /// Returns object index in world.
    int                     GetIndex() const { return index; }

                            /// Returns view count.
    int                     GetViewCount() const { return viewCount; }

                            /// Returns state.
    const State &           GetState() const { return state; }

                            /// Returns non-scaled AABB in local space.
    const AABB              GetLocalAABB() const;

                            /// Returns AABB in world space.
    const AABB &            GetWorldAABB() const { return worldAABB; }

                            /// Returns OBB in world space.
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns local to world matrix.
    const Mat3x4 &          GetWorldMatrix() const { return worldMatrix; }

                            /// Returns previous local to world matrix.
    const Mat3x4 &          GetPrevWorldMatrix() const { return prevWorldMatrix; }

private:
                            /// Updates this render object with the given state.
    void                    Update(const State *state);

    bool                    firstUpdate;

    State                   state;

    AABB                    worldAABB;
    OBB                     worldOBB;
    Mat3x4                  worldMatrix;
    Mat3x4                  prevWorldMatrix;
    float                   maxVisDistSquared;

    VisObject *             visObject;
    int                     viewCount;

    RenderWorld *           renderWorld;
    int                     index;                      // index of object list in RenderWorld
    DbvtProxy *             proxy;                      // proxy for render object

    int                     numMeshSurfProxies;         // number of proxies for static sub mesh
    DbvtProxy *             meshSurfProxies;            // proxies for static sub mesh
};

BE_NAMESPACE_END
