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

#include "RenderObject.h"

class RenderContext;
class RenderCamera;
class RenderFrameData;
class VisCamera;

/// Proxy node in the dynamic bounding volume tree
struct DbvtProxy {
    int32_t                         id;             ///< Proxy id
    BE1::AABB                       worldAABB;      ///< World bounding volume for this node
    RenderObject *                  renderObject;
};

class RenderWorld {
public:
    RenderWorld();

    void                            ClearScene();

    RenderObject *                  GetRenderObject(int handle) const;
    int                             AddRenderObject(const RenderObject::Decl &def);
    void                            UpdateRenderObject(int handle, const RenderObject::Decl &def);
    void                            RemoveRenderObject(int handle);

    void                            RenderScene(RenderContext *renderContext, const RenderCamera *renderCamera);

private:
    void                            FindVisObjects(const RenderCamera *renderCamera, VisCamera *visCamera, RenderFrameData *frameData);

    void                            DrawCamera(const RenderCamera *renderCamera, VisCamera *visCamera, RenderFrameData *frameData);

    BE1::Array<RenderObject *>      renderObjects;

#ifdef USE_DBVT
    BE1::DynamicAABBTree            objectDbvt;             ///< Dynamic bounding volume tree for render objects
#endif
};
