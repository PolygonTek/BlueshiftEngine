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

#include "RenderLight.h"
#include "RenderObject.h"
#include "Render/Mesh/GuiMesh.h"
#include "RenderPassBuilder.h"

struct VisSurf;
struct VisCamera;
struct VisLight;
struct VisObject;
struct RenderCameraDesc;
class RenderFrameData;
class RenderCamera;
class Texture;
class Material;
class Mesh;
class SubMesh;
enum class VisSurfFlag : uint8_t;

enum class DrawTextFlag : uint16_t {
    None                        = 0,
    Right                       = BIT(0),
    Center                      = BIT(1),
    Bottom                      = BIT(2),
    VCenter                     = BIT(3),
    MultiLines                  = BIT(5),
    WordWrap                    = BIT(6),
    Truncate                    = BIT(7),
    DropShadows                 = BIT(8),
    AddOutlines                 = BIT(9)
};

template<>
struct enable_bitmask_operators<DrawTextFlag> {
    static constexpr bool enable = true;
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
    enum class ClearFlag : uint8_t {
        None                    = 0,
        RenderObjects           = BIT(0),
        RenderLights            = BIT(1),
        All                     = BIT(0) | BIT(1)
    };

    RenderWorld();

    void                        BeginFrame();
    void                        EndFrame();

    void                        ClearScene(ClearFlag clearFlags = ClearFlag::All);

    RenderObject *              GetRenderObject(int index) const;
    int                         AddRenderObject(const RenderObject::Desc &desc);
    void                        UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags);
    void                        RemoveRenderObject(int index);

    RenderLight *               GetRenderLight(int index) const;
    int                         AddRenderLight(const RenderLight::Desc &desc);
    void                        UpdateRenderLight(int index, const RenderLight::Desc &desc, RenderLight::Desc::UpdateFlag updateFlags);
    void                        RemoveRenderLight(int index);

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
        VisObject *             firstVisObject = nullptr;
        uint32_t                numVisObjects = 0;
    };

    bool                        IsVisLightRegisteredInCurrentView(const RenderLight *renderLight) const;
    VisLight *                  RegisterVisLight(RenderFrameData *frameData, RenderLight *renderLight);
    void                        FindVisLights(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    static void                 ComputeLightTileRange(const VisCamera *visCamera, const BE1::Rect &screenClipRect, uint32_t &packedTileMin, uint32_t &packedTileMax);
    static void                 BuildLightCullingData(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    static void                 BuildLightZBins(RenderFrameData *frameData, const RenderCameraDesc &cameraDesc, VisCamera *visCamera);
    bool                        IsVisObjectRegisteredInCurrentView(const RenderObject *renderObject) const;
    VisObject *                 RegisterVisObject(RenderFrameData *frameData, RenderObject *renderObject);
    void                        CapturePrevWorldMatrix(RenderObject *renderObject);
    void                        SetVisObjectWorldMatrices(const RenderObject *renderObject, VisObject *visObject);
    void                        FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        UpdateVisibleSkinningJointCaches(RenderFrameData *frameData);
    void                        UpdateSkinningJointCaches(RenderFrameData *frameData, VisObject *firstVisObject, uint32_t numVisObjects, uint32_t numSkinningJoints);
    void                        UpdateSkinningJointCachesWithoutTask(RenderFrameData *frameData, VisObject *firstVisObject, uint32_t numVisObjects);
    void                        UpdateSkinningJointCachesWithTask(RenderFrameData *frameData, VisObject *firstVisObject, uint32_t numVisObjects, uint32_t numSkinningJoints, uint32_t numTasks);
    void                        UpdateSkinningJointCachesByTask(RenderWorld::SkinningJointCacheTaskDesc *taskDesc);
    static void                 UpdateSkinningJointCachesByTaskFunction(void *data);
    void                        AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddRawMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        SetupPyramidBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);
    void                        SetupConvolutionBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);

    void                        DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh);

    void                        AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags);

    BE1::Array<RenderObject *>  renderObjects;      /// The array of pointers to all render objects managed by the world.
    BE1::Array<RenderLight *>   renderLights;       /// The array of pointers to all render lights managed by the world.

    bool                        frameActive = false;
    uint64_t                    renderWorldFrameIndex = 0;
    uint64_t                    viewCount = 0;      ///< Counter used to track how many views (or scene renderings) have been issued.

    uint32_t                    renderWorldGroupId = -1;
    BE1::Array<SkinningJointCacheTaskDesc> skinningJointCacheTaskDescs;

    GuiMesh                     textMesh;           ///< 3D text mesh
    SharedPtr<Material>         skyboxMaterial;

    RenderPassBuilder           renderPassBuilder;

    BE1::Color4                 debugLineColor;
    BE1::Color4                 debugFillColor;
};

template<>
struct enable_bitmask_operators<RenderWorld::ClearFlag> {
    static constexpr bool enable = true;
};

BE_INLINE bool RenderWorld::IsVisLightRegisteredInCurrentView(const RenderLight *renderLight) const {
    // 현재 view 에 이미 등록된 VisLight 인지 체크
    return renderLight->viewCount == viewCount;
}

BE_INLINE bool RenderWorld::IsVisObjectRegisteredInCurrentView(const RenderObject *renderObject) const {
    // 현재 view 에 이미 등록된 VisObject 인지 체크
    return renderObject->viewCount == viewCount;
}
