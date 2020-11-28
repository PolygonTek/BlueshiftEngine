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
#include "Core/Vertex.h"
#include "Math/Math.h"
#include "Containers/Array.h"
#include "Font.h"

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
class RenderWorld;

class RenderObject {
    friend class RenderWorld;

public:
    struct Flag {
        enum Enum {
            UseRenderingOrder   = BIT(0),
            FirstPersonOnly     = BIT(1),
            ThirdPersonOnly     = BIT(2),
            NoVisDist           = BIT(3),
            Billboard           = BIT(4),
            DepthHack           = BIT(5),
            EnvProbeLit         = BIT(6),
            CastShadows         = BIT(7),
            ReceiveShadows      = BIT(8),
            Occluder            = BIT(9),   // for use in HOM
            SkipRendering       = BIT(10),
            SkipSelection       = BIT(11),
            RichText            = BIT(12),
        };
    };

    struct MaterialParm {
        enum Enum {
            Red,
            Green,
            Blue,
            Alpha,
            TimeOffset,
            TimeScale,
            Count       // should be less than MAX_EXPR_LOCALPARMS
        };
    };

    struct TextDrawMode {
        enum Enum {
            Normal,
            DropShadows,
            AddOutlines
        };
    };

    struct TextAnchor {
        enum Enum {
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
    };

    struct TextHorzAlignment {
        enum Enum {
            Left,
            Center,
            Right
        };
    };

    struct TextVertAlignment {
        enum Enum {
            Top,
            Middle,
            Bottom
        };
    };

    struct TextHorzOverflow {
        enum Enum {
            Wrap,
            Overflow
        };
    };

    struct TextVertOverflow {
        enum Enum {
            Truncate,
            Overflow
        };
    };

    struct WireframeMode {
        enum Enum {
            ShowNone,
            ShowVisibleFront,
            ShowAllFront,
            ShowAllFrontAndBack
        };
    };

    struct State {
        int                 flags = 0;
        int                 layer = 0;
        int                 staticMask = 0;
        int                 time = 0;
        float               maxVisDist = MeterToUnit(100);
        int                 renderingOrder = 0;

        //
        // Transform info
        //
        Mat3x4              worldMatrix = Mat3x4::identity;
        AABB                aabb = AABB::empty;          ///< non-scaled AABB in local space

        //
        // Wireframe info
        //
        WireframeMode::Enum wireframeMode = WireframeMode::ShowNone;
        Color4              wireframeColor = Color4::white;

        //
        // static/skinned mesh
        //
        Mesh *              mesh = nullptr;             ///< Instantiated mesh
        const Skeleton *    skeleton = nullptr;         ///< Skeleton information for skeletal mesh
        int                 numJoints = 0;              ///< Number of joints
        Mat3x4 *            joints = nullptr;           ///< Joint transform matrices to animate skeletal mesh

        //
        // Raw indexed triangles rendering (typically for GUI rendering)
        //
        int                 numVerts = 0;
        VertexGeneric *     verts = nullptr;
        int                 numIndexes = 0;
        TriIndex *          indexes = nullptr;

        //
        // Text rendering
        //
        Font *              font = nullptr;
        Str                 text;                       ///< UTF8 encoded string
        TextDrawMode::Enum  textDrawMode = TextDrawMode::Normal;
        Color4              textFxColor = Color4::black;
        TextAnchor::Enum    textAnchor = TextAnchor::UpperLeft;
        TextHorzAlignment::Enum textHorzAlignment = TextHorzAlignment::Center;
        TextVertAlignment::Enum textVertAlignment = TextVertAlignment::Middle;
        RectF               textRect = RectF::zero;
        TextHorzOverflow::Enum textHorzOverflow = TextHorzOverflow::Wrap;
        TextVertOverflow::Enum textVertOverflow = TextVertOverflow::Truncate;
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
        float               materialParms[MaterialParm::Count] = { 1, 1, 1, 1, 0, 1 };
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

                            /// Returns AABB in world space.
    const AABB &            GetWorldAABB() const { return worldAABB; }

                            /// Returns OBB in world space.
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns local to world matrix.
    const Mat3x4 &          GetWorldMatrix() const { return worldMatrix; }

                            /// Returns world to local matrix.
    const Mat3x4 &          GetWorldMatrixInverse() const { return worldMatrixInverse; }

                            /// Returns previous local to world matrix.
    const Mat3x4 &          GetPrevWorldMatrix() const { return prevWorldMatrix; }

private:
                            /// Updates this render object with the given state.
    void                    Update(const State *state);

    State                   state;

    AABB                    worldAABB = AABB::empty;
    OBB                     worldOBB;
    Mat3x4                  worldMatrix = Mat3x4::identity;
    Mat3x4                  worldMatrixInverse = Mat3x4::identity;
    Mat3x4                  prevWorldMatrix = Mat3x4::identity;

    float                   maxVisDistSquared;

    VisObject *             visObject = nullptr;
    int                     viewCount = 0;

    RenderWorld *           renderWorld;
    int                     index;                      // index of object list in RenderWorld
    DbvtProxy *             proxy = nullptr;            // proxy for render object

    int                     numMeshSurfProxies = 0;     // number of proxies for static sub mesh
    DbvtProxy *             meshSurfProxies = nullptr;  // proxies for static sub mesh
};

BE_NAMESPACE_END
