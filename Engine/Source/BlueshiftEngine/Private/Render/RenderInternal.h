#pragma once

BE_NAMESPACE_BEGIN

struct viewEntity_t {
    const SceneEntity *     def;

    viewEntity_t *          next;

    Mat4                    modelViewMatrix;
    Mat4                    modelViewProjMatrix;

    bool                    ambientVisible;
    bool                    shadowVisible;
};

struct drawSurfNode_t {
    const DrawSurf *        drawSurf;

    drawSurfNode_t *        next;
};

struct viewLight_t {
    const SceneLight *      def;

    viewLight_t *           next;

    float *                 materialRegisters;

    Rect                    scissorRect;        // y 좌표는 밑에서 부터 증가
    bool                    occlusionVisible;

    OBB                     mainLightOBB;
    Mat4                    viewProjTexMatrix;

    Color4                  lightColor;

                            // light bounding volume 에 포함되고, view frustum 에 보이는 surfaces
    drawSurfNode_t *        litSurfs;
    AABB                    litAABB;

                            // light bounding volume 에 포함되고, shadow caster 가 view frustum 에 보이는 surfaces (litSurfs 를 포함한다)
    drawSurfNode_t *        shadowCasterSurfs;
    AABB                    shadowCasterAABB;
};

struct view_t {
    const SceneView *       def;

    bool                    isSubview;
    bool                    isMirror;
    bool                    is2D;

    AABB                    aabb;

                            // view 에 보이는 모든 surfaces 와 shadow surfaces
    DrawSurf **             drawSurfs;
    int                     numDrawSurfs;
    int                     maxDrawSurfs;

    viewEntity_t *          viewEntities;
    viewLight_t *           viewLights;
};

struct renderGlobal_t {
    int                     skinningMethod;
    int                     vtUpdateMethod;          // vertex texture update method
};

extern renderGlobal_t       renderGlobal;

BE_NAMESPACE_END

#include "VertexFormat.h"
#include "RenderTarget.h"
#include "DrawSurf.h"
#include "RenderCmd.h"
#include "RenderPostProcess.h"
#include "RenderCVars.h"
#include "RenderUtils.h"
#include "FrameData.h"
#include "RBackEnd.h"
