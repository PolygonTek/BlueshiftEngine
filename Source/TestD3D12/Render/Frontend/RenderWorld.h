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

#include "EnvironmentLight.h"
#include "OceanSurface.h"
#include "RenderLight.h"
#include "RenderObject.h"
#include "Render/Mesh/GuiMesh.h"
#include "RenderPassBuilder.h"
#include "Render/DebugDrawPipeline.h"
#include "Render/DebugDrawVertex.h"

struct VisSurf;
struct VisCamera;
struct VisLight;
struct VisObject;
struct DebugDrawData;
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
struct BE1::enable_bitmask_operators<DrawTextFlag> {
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
    friend class ProbeBakeSystem;

public:
    enum class ClearFlag : uint8_t {
        None                    = 0,
        RenderObjects           = BIT(0),
        RenderLights            = BIT(1),
        All                     = BIT(0) | BIT(1)
    };

    friend constexpr std::true_type EnableBitmaskEnum(ClearFlag) noexcept { return {}; }

    RenderWorld();
    ~RenderWorld();

    void                        BeginFrame();
    void                        EndFrame();

                                /// 프레임이 활성화되어 있으면 현재 프레임 인덱스를, 그렇지 않으면 다음 프레임 인덱스를 반환한다.
    uint64_t                    GetCurrentOrNextFrameIndex() const { return frameActive ? frameIndex : frameIndex + 1; }
    uint64_t                    GetCurrentFrameIndex() const { return frameIndex; }

    void                        ClearScene(ClearFlag clearFlags = ClearFlag::All);

    RenderObject *              GetRenderObject(int index) const;
    int                         AddRenderObject(const RenderObject::Desc &desc);
    void                        UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags);
    void                        UpdateRenderObjectFlags(int index, RenderObject::Flag flags);
    void                        UpdateRenderObjectLayer(int index, uint8_t layer);
    void                        UpdateRenderObjectTransform(int index, const BE1::Mat3x4 &worldMatrix);
    void                        UpdateRenderObjectBounds(int index, const BE1::AABB &aabb);
    void                        UpdateRenderObjectMesh(int index, const SharedPtr<MeshInstance> &meshInstance);
    void                        UpdateRenderObjectMaterials(int index, const BE1::Array<SharedPtr<Material>> &materials);
    void                        UpdateRenderObjectFont(int index, const SharedPtr<Font> &font);
    void                        UpdateRenderObjectText(int index, const BE1::Str &text);
    void                        UpdateRenderObjectTextStyle(int index, const RenderObject::TextDesc &textDesc);
    void                        RemoveRenderObject(int index);

    RenderLight *               GetRenderLight(int index) const;
    int                         AddRenderLight(const RenderLight::Desc &desc);
    void                        UpdateRenderLight(int index, const RenderLight::Desc &desc, RenderLight::Desc::UpdateFlag updateFlags);
    void                        UpdateRenderLightFlags(int index, RenderLight::Flag flags);
    void                        UpdateRenderLightLayer(int index, uint8_t layer);
    void                        UpdateRenderLightTransform(int index, const BE1::Vec3 &origin, const BE1::Mat3 &axis);
    void                        UpdateRenderLightShape(int index, RenderLight::Type type, const BE1::Vec3 &size, float zNear);
    void                        UpdateRenderLightProjectionPolicy(int index, const RHI::ProjectionPolicy &projectionPolicy);
    void                        UpdateRenderLightLighting(int index, const BE1::Color3 &linearColor, float intensity, float fallOffExponent);
    void                        UpdateRenderLightShadow(int index, float shadowOffsetFactor, float shadowOffsetUnits);
    void                        RemoveRenderLight(int index);

    const EnvironmentLight *    GetEnvironmentLight() const { return hasEnvironmentLight ? &environmentLight : nullptr; }
    bool                        AddEnvironmentLight(const EnvironmentLight::Desc &desc);
    void                        UpdateEnvironmentLight(const EnvironmentLight::Desc &desc);
    bool                        SetEnvironmentLightBakedCubeMaps(const SharedPtr<Texture> &diffuseIrradianceCubeMap, const SharedPtr<Texture> &specularPrefilteredCubeMap);
    void                        RequestEnvironmentLightCubemapRegeneration(const BE1::Vec3 &captureOrigin = BE1::Vec3::origin);
    void                        RemoveEnvironmentLight();

    const OceanSurface *        GetOceanSurface() const { return hasOceanSurface ? &oceanSurface : nullptr; }
    bool                        AddOceanSurface(const OceanSurface::Desc &desc);
    bool                        UpdateOceanSurface(const OceanSurface::Desc &desc);
    void                        RemoveOceanSurface();
    bool                        SetOceanSurfaceQueries(const OceanSurfaceQueryInput *queries, uint32_t queryCount);

    void                        SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial);

    void                        RenderScene(RenderCamera *renderCamera);
    void                        RenderGUI(GuiMesh &guiMesh);

    void                        SetDebugColor(const BE1::Color4 &lineColor, const BE1::Color4 &fillColor);
    void                        ClearDebugPrimitives();

                                /// 디버그 프리미티브는 메인 스레드에서만 등록할 수 있다. 프레임이 활성화되지 않은 상태에서 등록하면 다음 프레임을 대상으로 한다.
                                /// lifeTime은 밀리초 단위이며 0 이상이어야 한다. 0 이면 해당 프리미티브는 대상 프레임 동안만 유지된다.
    void                        DrawDebugPoint(const BE1::Vec3 &origin, float size, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugLine(const BE1::Vec3 &start, const BE1::Vec3 &end, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugPolyline(const BE1::Vec3 *points, uint32_t numPoints, bool closed = false, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugAxis(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float size, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugTriangle(const BE1::Vec3 &a, const BE1::Vec3 &b, const BE1::Vec3 &c, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugQuad(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, float size, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugPlane(const BE1::Vec3 &origin, const BE1::Vec3 &normal, float size, bool depthTest = false, int lifeTime = 0);
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
    void                        DrawDebugConeSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinder(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinder(const BE1::Vec3 &top, const BE1::Vec3 &bottom, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCylinderSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsule(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsule(const BE1::Vec3 &a, const BE1::Vec3 &b, float radius, bool twoSided = true, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugCapsuleSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest = false, int lifeTime = 0);
    void                        DrawDebugArrow(const BE1::Vec3 &start, const BE1::Vec3 &end, float coneSize, float coneRadius, bool depthTest = false, int lifeTime = 0);

private:
    enum class SkyboxState : uint8_t {
        None,
        Pending,
        Ready
    };

    struct DebugPrimitiveRange {
        uint32_t                firstVertex = 0;
        uint32_t                numVerts = 0;
        uint64_t                expireFrameIndex = ~uint64_t(0);
        uint64_t                expireTimeUsec = 0;
    };

    struct DebugPrimitiveBucket {
        DebugPrimitiveBucket() : vertices(256), primitives(64) {}

        BE1::Array<DebugDrawVertex> vertices;
        BE1::Array<DebugPrimitiveRange> primitives;
    };

    struct SkinningJointCacheTaskDesc {
        int32_t                 threadIndex = -1;
        RenderWorld *           renderWorld = nullptr;
        RenderFrameData *       frameData = nullptr;
        VisObject *             firstVisObject = nullptr;
        uint32_t                numVisObjects = 0;
    };

    bool                        IsVisLightRegisteredInCurrentView(const RenderLight *renderLight) const;
    VisLight *                  RegisterVisLight(RenderFrameData *frameData, RenderLight *renderLight);
    VisObject *                 RegisterVisObject(RenderFrameData *frameData, RenderObject *renderObject);
    RenderObject *              GetRenderObjectForUpdate(int index, const char *functionName) const;
    RenderLight *               GetRenderLightForUpdate(int index, const char *functionName) const;
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
    SkyboxState                 GetSkyboxState() const;
    void                        AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        FindVisLights(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    static void                 ComputeLightTileRange(const VisCamera *visCamera, const BE1::Rect &screenClipRect, uint32_t &packedTileMin, uint32_t &packedTileMax);
    static void                 BuildLightCullingData(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    static void                 BuildLightZBins(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    bool                        IsVisObjectRegisteredInCurrentView(const RenderObject *renderObject) const;
    void                        SetupPyramidBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);
    void                        SetupConvolutionBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera);

    DebugDrawVertex *           ReserveDebugVertices(DebugDrawPipelineId pipelineId, uint32_t numVerts, const BE1::Color4 &color, int lifeTime);
    DebugDrawVertex *           ReserveDebugLineVertices(uint32_t numVerts, bool depthTest, int lifeTime);
    DebugDrawVertex *           ReserveDebugFillVertices(uint32_t numVerts, bool twoSided, bool depthTest, int lifeTime);
    void                        ExpireDebugPrimitives(uint64_t currentTimeUsec);
    void                        BuildDebugDraw(RenderFrameData *frameData, VisCamera *visCamera);
    const DebugDrawData *       BuildDebugDrawData(RenderFrameData *frameData);
    void                        PrepareDebugDrawPipelines(VisCamera *visCamera);
    void                        PrepareOceanSurfacePipeline(VisCamera *visCamera);

    void                        DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh);

    void                        AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags);

    BE1::Array<RenderObject *>  renderObjects;      /// The array of pointers to all render objects managed by the world.
    BE1::Array<RenderLight *>   renderLights;       /// The array of pointers to all render lights managed by the world.
    EnvironmentLight            environmentLight;
    bool                        hasEnvironmentLight = false;
    OceanSurface                oceanSurface;
    bool                        hasOceanSurface = false;

    bool                        frameActive = false;
    uint64_t                    frameIndex = 0;
    uint64_t                    viewCount = 0;      ///< Counter used to track how many views (or scene renderings) have been issued.

    uint32_t                    renderWorldGroupId = -1;
    BE1::Array<SkinningJointCacheTaskDesc> skinningJointCacheTaskDescs;

    GuiMesh                     textMesh;           ///< 3D text mesh
    SharedPtr<Material>         skyboxMaterial;

    RenderPassBuilder           renderPassBuilder;

    BE1::Color4                 debugLineColor;     ///< Current debug line color
    BE1::Color4                 debugFillColor;     ///< Current debug fill color
    DebugPrimitiveBucket        debugPrimitiveBuckets[static_cast<int>(DebugDrawPipelineId::Count)];
    uint32_t                    debugPrimitiveCount = 0;
    uint32_t                    debugPrimitiveVertexCount = 0;
    uint64_t                    debugCapacityWarningFrameIndex = ~uint64_t(0);
};

BE_INLINE bool RenderWorld::IsVisLightRegisteredInCurrentView(const RenderLight *renderLight) const {
    // 현재 view 에 이미 등록된 VisLight 인지 체크
    return renderLight->viewCount == viewCount;
}

BE_INLINE bool RenderWorld::IsVisObjectRegisteredInCurrentView(const RenderObject *renderObject) const {
    // 현재 view 에 이미 등록된 VisObject 인지 체크
    return renderObject->viewCount == viewCount;
}
