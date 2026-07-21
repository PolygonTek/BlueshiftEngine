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

#include "Precompiled.h"

class RenderContext;
class RenderWorld;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Render Capture

---------------------------------------------------------------------------------------------------
*/

class RenderCapture {
public:
                    // 지정된 텍스쳐에 씬을 렌더링하는 독립 프레임을 제출한다.
    static bool     CaptureSceneToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
                        const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, int width, int height, const SharedPtr<Texture> &targetTexture, bool waitForCompletion = false);

                    // 씬을 임시 렌더 타겟에 렌더링하고 CPU 이미지로 읽어온다.
    static bool     CaptureSceneToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
                        const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height, BE1::Image &outImage);

                    // cubemap 의 지정된 face 에 씬을 렌더링하는 독립 프레임을 제출한다.
    static bool     CaptureEnvCubeFaceToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                        const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, RHI::CubemapFace face, bool waitForCompletion = false);

                    // cubemap 의 여섯 face 에 씬을 하나의 독립 프레임으로 렌더링한다.
    static bool     CaptureEnvCubeToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                        const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, bool waitForCompletion = false);

                    // 씬을 임시 cubemap 렌더 타겟에 렌더링하고 CPU 이미지로 읽어온다.
    static bool     CaptureEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                        const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, BE1::Image &outImage);

                    // 씬을 캡처해서 이미지 파일로 저장한다. 확장자가 없으면 png 를 사용한다.
    static bool     TakeScreenShot(const char *filename, RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask,
                        const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height);

                    // 환경 cubemap 을 캡처해서 이미지 파일로 저장한다. 확장자가 없으면 dds 를 사용한다.
    static bool     TakeEnvShot(const char *filename, RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
                        const BE1::Vec3 &origin, bool useHDR, int size = 256);
};
