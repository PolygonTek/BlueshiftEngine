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

#include "Core/DynamicAABBTree.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Render World

-------------------------------------------------------------------------------
*/

class DrawSurf;
class VisCamera;

/// Proxy node in the dynamic bounding volume tree
struct DbvtProxy {
    int32_t                     id;             ///< Proxy id
    AABB                        worldAABB;      ///< World bounding volume for this node
    RenderObject *              renderObject;
    RenderLight *               renderLight;
    //ReflectionProbe *         localProbe;
    Mesh *                      mesh;           ///< Static mesh pointer
    int32_t                     meshSurfIndex;  ///< Sub mesh index
};

class RenderWorld {
    friend class RenderSystem;
    friend class RenderContext;

public:
    RenderWorld();
    ~RenderWorld();

    void                        ClearScene();

                                /// Adds render object to this world.
    int                         AddRenderObject(const RenderObject::State *def);

                                /// Updates render object.
    void                        UpdateRenderObject(int handle, const RenderObject::State *def);

                                /// Removes render object.
    void                        RemoveRenderObject(int handle);

                                /// Gets RenderObject pointer by given render object handle.
    const RenderObject *        GetRenderObject(int handle) const;

                                /// Adds render light to this world.
    int                         AddRenderLight(const RenderLight::State *def);

                                /// Updates render light.
    void                        UpdateRenderLight(int handle, const RenderLight::State *def);

                                /// Removes render light.
    void                        RemoveRenderLight(int handle);

                                /// Gets RenderLight pointer by given render light handle.
    const RenderLight *         GetRenderLight(int handle) const;

    int                         GetViewCount() const { return viewCount; }

    void                        RenderScene(const RenderCamera *camera);

    void                        SetSkyboxMaterial(Material *skyboxMaterial);

    const AABB &                GetStaticAABB() const { return staticMeshDbvt.GetRootFatAABB(); }

    const GuiMesh &             GetTextMesh() const { return textMesh; }

    void                        FinishMapLoading();

                                /// Set color for the debug primitives.
    void                        SetDebugColor(const Color4 &lineColor, const Color4 &fillColor) { debugLineColor = lineColor; debugFillColor = fillColor; }

    void                        ClearDebugPrimitives(int time);

    void                        DebugLine(const Vec3 &start, const Vec3 &end, float lineWidth, bool depthTest = false, int lifeTime = 0);
    void                        DebugTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugQuad(const Vec3 &origin, const Vec3 &right, const Vec3 &up, float size, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCircle(const Vec3 &origin, const Vec3 &dir, const float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugArc(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugEllipse(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius1, const float radius2, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugHemisphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);	
    void                        DebugHemisphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth = 1, bool depthTest = false, int lifeTime = 0);	
    void                        DebugSphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugSphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth = 1, bool depthTest = false, int lifeTime = 0);
    void                        DebugAABB(const AABB &aabb, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugOBB(const OBB &obb, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugFrustum(const Frustum &frustum, bool showFromOrigin, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCone(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCone(const Vec3 &apex, const Vec3 &bottom, float radius1, float radius2, bool drawCap, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugConeSimple(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCylinder(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCylinder(const Vec3 &top, const Vec3 &bottom, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);	
    void                        DebugCylinderSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth = 1, bool depthTest = false, int lifeTime = 0);	
    void                        DebugCapsule(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCapsule(const Vec3 &a, const Vec3 &b, float radius, float lineWidth = 1, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DebugCapsuleSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth = 1, bool depthTest = false, int lifeTime = 0);
    void                        DebugArrow(const Vec3 &start, const Vec3 &end, float coneSize, float coneRadius, float lineWidth = 1, bool depthTest = false, int lifeTime = 0);

    void                        ClearDebugText(int time);
    void                        DebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth = 1, const int align = 1, bool depthTest = false, int lifeTime = 0);

    void                        DebugJoints(const RenderObject *object, bool showJointsNames, const Mat3 &viewAxis);

private:
    VisObject *                 RegisterVisObject(VisCamera *camera, RenderObject *object);
    VisLight *                  RegisterVisLight(VisCamera *camera, RenderLight *light);
    void                        FindVisLightsAndObjects(VisCamera *camera);
    void                        AddStaticMeshes(VisCamera *camera);
    void                        AddSkinnedMeshes(VisCamera *camera);
    void                        AddParticleMeshes(VisCamera *camera);
    void                        AddTextMeshes(VisCamera *camera);
    void                        AddSkyBoxMeshes(VisCamera *camera);
    void                        AddStaticMeshesForLights(VisCamera *camera);
    void                        AddSkinnedMeshesForLights(VisCamera *camera);
    void                        AddSubCamera(VisCamera *camera);
    void                        CacheInstanceBuffer(VisCamera *camera);
    void                        OptimizeLights(VisCamera *camera);
    void                        AddDrawSurf(VisCamera *camera, VisLight *light, VisObject *entity, const Material *material, SubMesh *subMesh, int flags);
    void                        AddDrawSurfFromAmbient(VisCamera *camera, const VisLight *light, bool shadowVisible, const DrawSurf *ambientDrawSurf);
    void                        SortDrawSurfs(VisCamera *camera);

    void                        DrawCamera(VisCamera *camera);
    void                        DrawSubCamera(const VisObject *object, const DrawSurf *drawSurf, const Material *material);
    void                        DrawGUICamera(GuiMesh &guiMesh);

    Color4                      color;
    Color4                      clearColor;
    float                       clearDepth;

    Color4                      debugLineColor;
    Color4                      debugFillColor;

    VisCamera *                 currentCamera;
    int                         viewCount;

    Material *                  skyboxMaterial;
    ParticleMesh                particleMesh;       ///< particle mesh
    GuiMesh                     textMesh;           ///< 3D text mesh

    Array<RenderObject *>       renderObjects;      ///< Array of render objects
    Array<RenderLight *>        renderLights;       ///< Array of render lights
    //Array<ReflectionProbe *>  localProbes;        ///< Array of local light probes

    //ReflectionProbe *         globalProbe;        ///< Global light probe

    DynamicAABBTree             objectDbvt;         ///< Dynamic bounding volume tree for render objects
    DynamicAABBTree             lightDbvt;          ///< Dynamic bounding volume tree for render lights and reflection probes
    DynamicAABBTree             staticMeshDbvt;     ///< Dynamic bounding volume tree for static meshes
};

BE_NAMESPACE_END
