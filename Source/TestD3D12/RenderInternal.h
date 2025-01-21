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

#include "RHI.h"
#include "RenderObject.h"
#include "RenderCamera.h"

class RenderContext;
class VisCamera;

class VisObject {
public:
    RenderObject::Decl &        GetDecl() { return decl; }

    static void                 Draw(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObject);
    static void                 DrawInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObjects, int instanceCount);

    static void                 DrawCubeMesh(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObject);
    static void                 DrawCubeMeshInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObjects, int instanceCount);

    RenderObject::Decl          decl;

    ALIGN_AS32 BE1::Mat4        modelViewProjMatrix;
    ALIGN_AS32 BE1::Mat3x4      modelViewMatrix;

    bool                        ambientVisible = false;
    bool                        shadowVisible = false;
};

class VisCamera {
public:
    RenderCamera::Decl &        GetDecl() { return decl; }

    uint32_t                    NumVisObjects() const { return visObjectEndIndex - visObjectStartIndex + 1; }

    RenderCamera::Decl          decl;

    ALIGN_AS32 BE1::Mat4        viewProjMatrix;
    ALIGN_AS32 BE1::AABB        worldAABB;

    uint32_t                    visObjectStartIndex = 0;
    uint32_t                    visObjectEndIndex = -1;
};
