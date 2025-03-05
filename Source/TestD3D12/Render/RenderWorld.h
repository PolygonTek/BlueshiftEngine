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
#include "DrawSurf.h"
#include "GuiMesh.h"

class RenderFrameData;
class RenderContext;
class RenderCamera;
class VisCamera;
class VisLight;
class VisObject;
class Texture;
class Mesh;
class SubMesh;

/// Proxy node in the dynamic bounding volume tree.
struct DbvtProxy {
    BE1::AABB                   worldAABB;              ///< World bounding volume for this proxy node.
    RenderObject *              renderObject;           ///< Associated render object.
    Mesh *                      staticMesh;             ///< Static mesh.
    int32_t                     staticMeshSurfIndex;    ///< Index into the mesh's surface array.
    int32_t                     id;                     ///< Proxy id.
};

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
    static const bool enable = true;
};

/// RenderWorld is the central manager for scene objects.
/// It maintains a collection of render objects, organizes them spatially.
class RenderWorld {
public:
    RenderWorld();

    void                        ClearScene();

    RenderObject *              GetRenderObject(int index) const;
    int                         AddRenderObject(const RenderObject::Desc &desc);
    void                        UpdateRenderObject(int index, const RenderObject::Desc &desc);
    void                        RemoveRenderObject(int index);

    void                        RenderScene(const RenderCamera *renderCamera);
    void                        RenderGUI(GuiMesh &guiMesh);

private:
    bool                        IsVisObjectRegistered(const RenderObject *renderObject) const;
    VisObject *                 RegisterVisObject(RenderFrameData *frameData, const RenderObject *renderObject);
    void                        FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddStaticMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);

    void                        DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera);
    void                        DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh);

    void                        AddDrawSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Texture *texture, SubMesh *subMesh, DrawSurf::Flag flags);
    void                        SortDrawSurfs(VisCamera *visCamera);

    BE1::Array<RenderObject *>  renderObjects;      /// The array of pointers to all render objects managed by the world.

    uint32_t                    viewCount = 0;      ///< Counter used to track how many views (or scene renderings) have been issued.

#ifdef USE_DBVT
    BE1::DynamicAABBTree        objectDbvt;         ///< Dynamic bounding volume tree (DBVT) for render objects.
    BE1::DynamicAABBTree        staticMeshDbvt;     ///< Dynamic bounding volume tree (DBVT) for static meshes.
#endif

    GuiMesh                     textMesh;           ///< 3D text mesh
};
