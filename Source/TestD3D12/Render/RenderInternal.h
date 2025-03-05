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
#include "Font.h"
#include "Mesh.h"
#include "DrawSurf.h"

// 한 프레임 렌더링에서만 유효한 렌더링 카메라
// 매 프레임 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
class VisCamera {
public:
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

    bool                        is2D;
};

// 한 프레임 렌더링에서만 유효한 렌더링 객체.
// 매 프레임 외부 렌더링 객체로부터 복사되어 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
// 외부에서 공유받은 자원은 렌더링할 동안은 내부적으로 유지되어야 한다.
// 따라서 공유 자윈이 파괴되는 걸 방지하기 위해, 한 프레임 렌더링이 끝난 후 릴리즈하는 구조로 되어있다.
class VisObject {
public:
    ~VisObject() {
        if (mesh) {
            meshManager.ReleaseMesh(mesh);
        }
        for (int i = 0; i < numTextures; ++i) {
            textureManager.ReleaseTexture(textures[i]);
        }
        if (font) {
            fontManager.ReleaseFont(font);
        }
    }

    uint32_t                    index = ~0;

    ALIGN_AS32 BE1::Mat3x4      worldMatrix;
    ALIGN_AS32 BE1::Mat3x4      modelViewMatrix;
    ALIGN_AS32 BE1::Mat4        modelViewProjMatrix;

    Mesh *                      mesh = nullptr;
    uint32_t                    numTextures = 0;
    Texture *                   textures[32] = {};

    Font *                      font = nullptr;
    const char *                text = nullptr;
    BE1::RectF                  textRect;
    BE1::Color4                 textShadowColor;
    BE1::Vec2                   textShadowOffset;
    RenderObject::TextParams    textParams;
    float                       textScale;
    float                       textLineSpacing;

    bool                        ambientVisible = false;
    bool                        shadowVisible = false;
};

class VisLight {
public:
    uint32_t                    index = ~0;
};
