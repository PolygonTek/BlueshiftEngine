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

#include "Containers/LinkList.h"

BE_NAMESPACE_BEGIN

class VisibleObject {
public:
    int                     index;

    const RenderObject *    def;

    Mat3x4                  modelViewMatrix;
    Mat4                    modelViewProjMatrix;

    int                     instanceIndex;

    bool                    ambientVisible;
    bool                    shadowVisible;

    LinkList<VisibleObject> node;
};

class VisibleLight {
public:
    int                     index;

    const RenderLight *     def;

    float *                 materialRegisters;

    Rect                    scissorRect;        // y 좌표는 밑에서 부터 증가
    bool                    occlusionVisible;

    Mat4                    viewProjTexMatrix;

    Color4                  lightColor;

    int                     firstDrawSurf;
    int                     numDrawSurfs;

                            // light bounding volume 에 포함되고, view frustum 에 보이는 surfaces
    AABB                    litSurfsAABB;

                            // light bounding volume 에 포함되고, shadow caster 가 view frustum 에 보이는 surfaces (litSurfs 를 포함한다)
    AABB                    shadowCasterAABB;

    LinkList<VisibleLight>  node;
};

class VisibleView {
public:
    const RenderView *      def;

    bool                    isSubview;
    bool                    isMirror;
    bool                    is2D;

    AABB                    worldAABB;

    int                     numDrawSurfs;
    int                     maxDrawSurfs;
                            // view 에 보이는 모든 surfaces 와 shadow surfaces
    DrawSurf **             drawSurfs;

    int                     numAmbientSurfs;

    int                     numVisibleObjects;
    int                     numVisibleLights;

    BufferCache *           instanceBufferCache;

    LinkList<VisibleObject> visObjects;
    LinkList<VisibleLight>  visLights;
    VisibleLight *          primaryLight;
};

struct renderGlobal_t {
    int                     skinningMethod;
    int                     vtUpdateMethod;          // vertex texture update method
    int                     instancingMethod;
    int                     instanceBufferOffsetAlignment;
    void *                  instanceBufferData;
};

extern renderGlobal_t       renderGlobal;

void    RB_DrawRect(float x, float y, float x2, float y2, float s, float t, float s2, float t2);
void    RB_DrawClipRect(float s, float t, float s2, float t2);
void    RB_DrawRectSlice(float x, float y, float x2, float y2, float s, float t, float s2, float t2, float slice);
void    RB_DrawScreenRect(float x, float y, float w, float h, float s, float t, float s2, float t2);
void    RB_DrawScreenRectSlice(float x, float y, float w, float h, float s, float t, float s2, float t2, float slice);
void    RB_DrawCircle(const Vec3 &origin, const Vec3 &left, const Vec3 &up, const float radius);
void    RB_DrawAABB(const AABB &aabb);
void    RB_DrawOBB(const OBB &obb);
void    RB_DrawFrustum(const Frustum &frustum);
void    RB_DrawSphere(const Sphere &sphere, int lats, int longs);

BE_NAMESPACE_END

#include "VertexFormat.h"
#include "RenderTarget.h"
#include "DrawSurf.h"
#include "RenderCmd.h"
#include "RenderPostProcess.h"
#include "RenderCVars.h"
#include "RenderUtils.h"
#include "FrameData.h"
