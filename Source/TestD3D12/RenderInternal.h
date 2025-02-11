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

#include "RHI.h"
#include "RenderObject.h"
#include "RenderCamera.h"
#include "Texture.h"
#include "Mesh.h"
#include "DrawSurf.h"

class VisCamera {
public:
    uint32_t                    NumVisObjects() const { return visObjectEndIndex - visObjectStartIndex + 1; }

    static constexpr int        MaxDrawSurfs = 0x4000;

    uint32_t                    index = ~0;

    uint32_t                    numDrawSurfs = 0;
    uint32_t                    maxDrawSurfs = 0;
    const DrawSurf **           drawSurfs = nullptr; // 카메라에 보이는 모든 surfaces 와 shadow surfaces

    BE1::Rect                   renderRect;
    RenderCameraClearMethod     clearMethod;
    BE1::Color4                 clearColor;
    ALIGN_AS32 BE1::Mat4        viewProjMatrix;
    ALIGN_AS32 BE1::AABB        worldAABB;

    uint32_t                    visObjectStartIndex = 0;
    uint32_t                    visObjectEndIndex = -1;

    bool                        is2D = false;
};

class VisObject {
public:
    ~VisObject() {
        if (mesh) {
            meshManager.ReleaseMesh(mesh);
            mesh = nullptr;
        }
        for (Texture *texture : textures) {
            textureManager.ReleaseTexture(texture);
        }
        textures.Clear();
    }

    uint32_t                    index = ~0;

    ALIGN_AS32 BE1::Mat3x4      worldMatrix;
    ALIGN_AS32 BE1::Mat4        modelViewProjMatrix;
    ALIGN_AS32 BE1::Mat3x4      modelViewMatrix;

    Mesh *                      mesh = nullptr;
    BE1::Array<Texture *>       textures;

    bool                        ambientVisible = false;
    bool                        shadowVisible = false;
};

class VisLight {
public:
    uint32_t                    index = ~0;
};
