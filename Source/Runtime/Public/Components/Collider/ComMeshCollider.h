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

#include "ComCollider.h"

BE_NAMESPACE_BEGIN

class Mesh;

class ComMeshCollider : public ComCollider {
public:
    OBJECT_PROTOTYPE(ComMeshCollider);

    ComMeshCollider();
    virtual ~ComMeshCollider();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    virtual void            CreateCollider() override;

    Guid                    GetMeshGuid() const { return meshGuid; }
    void                    SetMeshGuid(const Guid &meshGuid);

    Mesh *                  GetMesh() const;
    void                    SetMesh(const Mesh *mesh);

    bool                    IsConvex() const { return convex; }
    void                    SetConvex(bool convex);

protected:
    Guid                    meshGuid = GuidMapper::defaultMeshGuid;
    bool                    convex = false;
};

BE_NAMESPACE_END
