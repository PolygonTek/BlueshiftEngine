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
                        // 씬을 지정된 텍스쳐에 렌더링한다.
    static bool         RenderSceneToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, int width, int height, const SharedPtr<Texture> &targetTexture, bool waitForCompletion = false);

                        // 씬을 임시 렌더 타겟 텍스쳐에 렌더링하여 캡처하고, CPU 이미지로 읽어온다.
    static bool         CaptureSceneToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height, BE1::Image &outImage);

                        // CaptureSceneToImage 함수로 생성한 image 를 파일로 저장한다. 확장자가 없으면 png 를 사용한다.
    static bool         CaptureSceneToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height, const char *filename);

                        // 씬을 지정된 cubemap face 에 렌더링한다.
    static bool         RenderEnvCubeFaceToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, RHI::CubemapFace face, bool waitForCompletion = false);

                        // 씬을 지정된 cubemap 의 각 face 에 렌더링한다.
    static bool         RenderEnvCubeToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, bool waitForCompletion = false);

                        // 씬을 임시 cubemap 렌더 타겟 텍스쳐에 렌더링하여 캡처하고, CPU 이미지로 읽어온다.
    static bool         CaptureEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, BE1::Image &outImage);

                        // CaptureEnvCubeToImage 함수로 생성한 image 를 파일로 저장한다. 확장자가 없으면 dds 를 사용한다.
    static bool         CaptureEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, const char *filename);

                        // 씬을 임시 cubemap 렌더 타겟 텍스쳐에 렌더링하여 캡처하고, Lambert diffuse irradiance cubemap 으로 적분하여 CPU 이미지로 읽어온다.
    static bool         CaptureDiffuseIrradianceEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, BE1::Image &outImage);

                        // CaptureDiffuseIrradianceEnvCubeToImage 함수로 생성한 image 를 dds 파일로 저장한다.
    static bool         CaptureDiffuseIrradianceEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, const char *filename);

                        // 씬을 임시 cubemap 렌더 타겟 텍스쳐에 렌더링하여 캡처하고, specular prefiltered cubemap 의 전체 mip chain 을 생성하여 CPU 이미지로 읽어온다.
    static bool         CaptureSpecularPrefilteredEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, BE1::Image &outImage);

                        // CaptureSpecularPrefilteredEnvCubeToImage 함수로 생성한 image 를 dds 파일로 저장한다.
    static bool         CaptureSpecularPrefilteredEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
                            const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, const char *filename);
};
