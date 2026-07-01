// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "RenderObject.h"
#include "Render/Mesh/GuiMesh.h"
#include "RenderPassBuilder.h"

struct VisSurf;
struct VisCamera;
struct VisLight;
struct VisObject;
class RenderFrameData;
class RenderCamera;
class Texture;
class Material;
class Mesh;
class SubMesh;
enum class VisSurfFlag : uint8_t;

enum class DrawTextFlag : uint16_t {
    None = 0,
    Right = BIT(0),
    Center = BIT(1),
    Bottom = BIT(2),
    VCenter = BIT(3),
    MultiLines = BIT(5),
    WordWrap = BIT(6),
    Truncate = BIT(7),
    DropShadows = BIT(8),
    AddOutlines = BIT(9)
};

template<>
struct enable_bitmask_operators<DrawTextFlag> {
    static const bool enable = true;
};

/// Proxy node in the dynamic bounding volume tree.
struct DbvtProxy {
    BE1::AABB                   worldAABB;              ///< World bounding volume for this proxy node.
    RenderObject *              renderObject;           ///< Associated render object.
    int32_t                     id;                     ///< Proxy id.
};

/*
---------------------------------------------------------------------------------------------------

    Render World

    RenderWorld is the central manager for scene objects.
    It maintains a collection of render objects, organizes them spatially.

---------------------------------------------------------------------------------------------------
*/

class RenderWorld {
public:
    RenderWorld();

    void                        ClearScene();

    RenderObject *              GetRenderObject(int index) const;
    int                         AddRenderObject(const RenderObject::Desc &desc);
    void                        UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags);
    void                        RemoveRenderObject(int index);

    void                        SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial);

    void                        RenderScene(RenderCamera *renderCamera);
    void                        RenderGUI(GuiMesh &guiMesh);

    void                        DrawDebugLine(const BE1::Vec3 &start, const BE1::Vec3 &end, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugTriangle(const BE1::Vec3 &a, const BE1::Vec3 &b, const BE1::Vec3 &c, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugQuad(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, float size, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugHollowCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius1, const float radius2, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugArc(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugEllipse(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius1, const float radius2, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugHemisphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugHemisphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugSphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugSphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugAABB(const BE1::AABB &aabb, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugOBB(const BE1::OBB &obb, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugFrustum(const BE1::Frustum &frustum, bool showFromOrigin, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCone(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool drawCap, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCone(const BE1::Vec3 &apex, const BE1::Vec3 &bottom, float radius1, float radius2, bool drawCap, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugConeSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool drawCap, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinder(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinder(const BE1::Vec3 &top, const BE1::Vec3 &bottom, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinderSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsule(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsule(const BE1::Vec3 &a, const BE1::Vec3 &b, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsuleSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugArrow(const BE1::Vec3 &start, const BE1::Vec3 &end, float coneSize, float coneRadius, bool depthTest = false, int lifeTime = 0);

private:
    struct SkinningJointCacheTaskDesc {
        int32_t                 threadIndex = -1;
        RenderWorld *           renderWorld = nullptr;
        RenderFrameData *       frameData = nullptr;
        VisObject **            visObjects = nullptr;
        uint32_t                numVisObjects = 0;
    };

    bool                        IsVisObjectRegistered(const RenderObject *renderObject) const;
    VisObject *                 RegisterVisObject(RenderFrameData *frameData, const RenderObject *renderObject);
    void                        FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        UpdateVisibleSkinningJointCaches(RenderFrameData *frameData);
    void                        UpdateSkinningJointCaches(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints);
    void                        UpdateSkinningJointCachesWithoutTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects);
    void                        UpdateSkinningJointCachesWithTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints, uint32_t numTasks);
    void                        UpdateSkinningJointCachesByTask(RenderWorld::SkinningJointCacheTaskDesc *taskDesc);
    static void                 UpdateSkinningJointCachesByTaskFunction(void *data);
    void                        AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddRawMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        SetupPyramidBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);
    void                        SetupConvolutionBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);

#ifdef USE_DBVT
    void                        CreateObjectProxy(RenderObject *renderObject);
    void                        UpdateObjectProxy(RenderObject *renderObject, const BE1::Mat3x4 &worldMatrix, const BE1::Vec3 &displacementVector);
    void                        InsertObjectProxyToDbvt(RenderObject *renderObject);
    void                        RemoveObjectProxyFromDbvt(RenderObject *renderObject);
    void                        RegisterDynamicObject(RenderObject *renderObject);
    void                        UnregisterDynamicObject(RenderObject *renderObject);
    void                        RegisterPendingObjectProxy(RenderObject *renderObject);
    void                        UnregisterPendingObjectProxy(RenderObject *renderObject);
    void                        UpdatePendingObjectProxies();
    void                        UpdateSleepingObjectProxies();
#endif

    void                        DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh);

    void                        AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags);

    BE1::Array<RenderObject *>  renderObjects;      /// The array of pointers to all render objects managed by the world.

    uint32_t                    viewCount = 0;      ///< Counter used to track how many views (or scene renderings) have been issued.

#ifdef USE_DBVT
    BE1::DynamicAABBTree        objectDbvt;         ///< Dynamic bounding volume tree (DBVT) for render objects.
    BE1::Array<int>             pendingObjectProxyObjects;
    BE1::Array<int>             dynamicObjectIndices;
#endif

    uint32_t                    renderWorldGroupId = -1;
    BE1::Array<SkinningJointCacheTaskDesc> skinningJointCacheTaskDescs;

    GuiMesh                     textMesh;           ///< 3D text mesh
    SharedPtr<Material>         skyboxMaterial;

    RenderPassBuilder           renderPassBuilder;

    BE1::Color4                 debugLineColor;
    BE1::Color4                 debugFillColor;
};
