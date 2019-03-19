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
    struct Flag {
        enum Enum {
            Static              = BIT(0),
            FirstPersonOnly     = BIT(1),
            ThirdPersonOnly     = BIT(2),
            Billboard           = BIT(3),
            DepthHack           = BIT(4),
            EnvProbeLit         = BIT(5),
            CastShadows         = BIT(6),
            ReceiveShadows      = BIT(7),
            Occluder            = BIT(8),   // for use in HOM
            SkipSelection       = BIT(9),
            RichText            = BIT(10),
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

    struct TextAlignment {
        enum Enum {
            Left,
            Center,
            Right
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

        //
        // Transform info
        //
        Mat3x4              worldMatrix = Mat3x4::identity;
        AABB                aabb = AABB::zero;          ///< non-scaled AABB in local space

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
        // Text rendering
        //
        Font *              font = nullptr;
        Str                 text;                       ///< UTF8 encoded string
        TextAnchor::Enum    textAnchor = TextAnchor::UpperLeft;
        TextAlignment::Enum textAlignment = TextAlignment::Left;
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

    AABB                    worldAABB = AABB::zero;
    OBB                     worldOBB = OBB::zero;
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
