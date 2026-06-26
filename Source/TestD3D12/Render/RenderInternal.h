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

#include "RHI/RHI.h"
#include "VertexInputLayoutFactory.h"
#include "RenderObject.h"
#include "RenderCamera.h"
#include "Texture.h"
#include "Material.h"
#include "Font.h"
#include "Mesh.h"
#include "MeshInstance.h"

enum class VisSurfFlag : uint8_t {
    None                        = 0,
    CameraVisible               = BIT(0),           ///< Surface is visible by camera
    ShadowVisible               = BIT(1),           ///< Shadow of surface is visible by camera
    ShowWires                   = BIT(2),
    SkipSelection               = BIT(3),
    Skybox                      = BIT(4)
};

template<>
struct enable_bitmask_operators<VisSurfFlag> {
    static const bool enable = true;
};

struct VisSurf {
    uint32_t                    index = ~0u;
    const VisObject *           space = nullptr;
    const SubMesh *             subMesh = nullptr;
    const Material *            material = nullptr;
    ShaderFeatureSet            baseFeatureSet;
    uint32_t                    shaderMaterialIndex = ~0u;
    VisSurfFlag                 flags = VisSurfFlag::None;
};

struct PassDrawSurf {
    const VisSurf *             visSurf = nullptr;
    uint64_t                    sortKey = 0;

    VertexInputLayoutId         inputLayoutId = VertexInputLayoutId::XyzStColorNormalTangent;
    RHI::PipelineState *        pso = nullptr;
};

enum class RenderPassType : uint8_t {
    Skybox,
    MainOpaque,
    MainTransparent,
    DepthPrepass,
    Shadow,
    Gui
};

struct RenderPass {
    RenderPassType              type = RenderPassType::MainOpaque;
    ShaderPipelineDesc          pipelineDesc;
    ShaderFeatureSet            passFeatureSet;
    VertexInputLayoutId         baseInputLayoutId = VertexInputLayoutId::XyzStColorNormalTangent;

    uint32_t                    maxDrawSurfs = 0;
    uint32_t                    numDrawSurfs = 0;
    const PassDrawSurf **       drawSurfs = nullptr;
};

// 한 프레임 렌더링에서만 유효한 렌더링 카메라
// 매 프레임 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
struct VisCamera {
    static constexpr int        MaxVisSurfs = 0x10000;
    static constexpr int        MaxRenderPasses = 8;

    ALIGN_AS32 BE1::Mat4        viewMatrix;
    ALIGN_AS32 BE1::Mat4        viewProjMatrix;
    ALIGN_AS32 BE1::AABB        worldAABB;

    uint32_t                    index = ~0;

    uint32_t                    maxVisSurfs = 0;
    uint32_t                    numVisSurfs = 0;
    const VisSurf **            visSurfs = nullptr; // 카메라에 보이는 모든 surfaces 와 shadow surfaces

    uint32_t                    numRenderPasses = 0;
    RenderPass *                renderPasses = nullptr;

    BE1::Rect                   outputRect;
    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;
    RenderCameraClearMethod     clearMethod;
    RenderCameraClearColorMode  clearColorMode;
    BE1::Color4                 clearColor;
    RenderCameraFlag            flags = RenderCameraFlag::NoFlag;

    RHI::Texture *              colorTexture = nullptr;
    RHI::Texture *              colorMSAATexture = nullptr;
    RHI::Texture *              depthTexture = nullptr;
    RHI::Texture *              postProcessColorTexture = nullptr;
    RHI::Buffer *               luminanceBuffer = nullptr;

    RHI::Texture *              bloomDownTexture = nullptr;
    RHI::Texture *              bloomUpTexture = nullptr;
    uint32_t                    bloomMipWidths[RenderBloomDownMaxLevelCount] = {};
    uint32_t                    bloomMipHeights[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipSRVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipUAVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipSRVs[RenderBloomUpMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipUAVs[RenderBloomUpMaxLevelCount] = {};
    uint32_t                    bloomMipCount = 0;

    uint32_t                    fftBloomSize = 0;
    RHI::Buffer *               fftBloomScreenSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomScreenSpectrumBuffer = nullptr;
    RHI::Texture *              fftBloomKernelSourceTexture = nullptr;
    RHI::Buffer *               fftBloomKernelSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomKernelSpectrumBuffer = nullptr;
    RHI::Buffer *               fftBloomWorkBuffer = nullptr;
    RHI::Texture *              fftBloomConvolutionOutputTexture = nullptr;
    uint32_t                    fftBloomConvolutionOutputWidth = 0;
    uint32_t                    fftBloomConvolutionOutputHeight = 0;
    float                       fftBloomKernelRotation = 0.0f;
    float                       fftBloomKernelSizeScale = 1.0f;
    BE1::Vec2                   fftBloomKernelExtents = BE1::Vec2(1.0f, 1.0f);
    float                       fftBloomKernelNormalizationFactor = 1.0f;
    bool                        fftBloomKernelDirty = true;

    bool                        is2D;
};

// 한 프레임 렌더링에서만 유효한 렌더링 객체.
// 매 프레임 외부 렌더링 객체로부터 복사되어 생성되고, 생성될 때 어떠한 힙 할당도 일어나지 않는다.
// 외부에서 공유받은 자원은 렌더링할 동안은 내부적으로 유지되어야 한다.
// 따라서 공유 자윈이 파괴되는 걸 방지하기 위해, 한 프레임 렌더링이 끝난 후 릴리즈하는 구조로 되어있다.
struct VisObject {
    ALIGN_AS32 BE1::Mat3x4      worldMatrix;

    uint32_t                    index = ~0;

    MeshInstance *              meshInstance = nullptr;     ///< Mesh instance
    const Skeleton *            skeleton = nullptr;         ///< Skeleton information for skeletal mesh

    uint32_t                    numMaterials = 0;
    const Material *            materials[32] = {};

    int                         numVerts = 0;
    BE1::VertexGeneric *        verts = nullptr;
    int                         numIndexes = 0;
    BE1::VertIndex *            indexes = nullptr;

    Font *                      font = nullptr;
    const char *                text = nullptr;
    BE1::RectF                  textRect;
    uint32_t                    textShadowColor;
    BE1::Vec2                   textShadowOffset;
    RenderObject::TextParams    textParams;
    float                       textScale;
    float                       textLineSpacing;

    bool                        cameraVisible = false;
    bool                        shadowVisible = false;
};

struct VisLight {
    uint32_t                    index = ~0;
};
