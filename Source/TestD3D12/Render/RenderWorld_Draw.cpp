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

#include "Precompiled.h"
#include "RenderWorld.h"
#include "RenderCvars.h"
#include "RenderContext.h"
#include "RenderCamera.h"
#include "RenderInternal.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Texture/Texture.h"

static constexpr uint32_t MaxSkinningJointsPerTask = 1024;

static const Material *GetVisObjectMaterial(const VisObject *visObject, int materialIndex) {
    if (visObject->materials && materialIndex >= 0 && static_cast<uint32_t>(materialIndex) < visObject->numMaterials) {
        return visObject->materials[materialIndex];
    }
    return materialManager.defaultLitMaterial.get();
}

static ShaderFeatureSet BuildBaseFeatureSet(const Material *material, const SubMesh *subMesh) {
    ShaderFeatureSet featureSet;

    if (material->GetRenderingMode() == Material::RenderingMode::AlphaCutoff) {
        featureSet.EnableFeature(ShaderFeature::Perforated);
    }
    int maxVertexWeights = subMesh->MaxVertexWeights();
    if (maxVertexWeights == 8) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight8);
    } else if (maxVertexWeights == 4) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight4);
    } else if (maxVertexWeights == 1) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight1);
    }

    return featureSet;
}

void RenderWorld::AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags) {
    if (visCamera->numVisSurfs + 1 > visCamera->maxVisSurfs) {
        BE_WARNLOG("RenderWorld::AddVisSurf: not enough available visible surfaces\n");
        return;
    }

    assert(material);

    VisSurf *drawSurf = (VisSurf *)frameData->MemAlloc(sizeof(VisSurf));
    drawSurf->index = visCamera->numVisSurfs;
    drawSurf->space = visObject;
    drawSurf->subMesh = subMesh;
    drawSurf->material = material;
    drawSurf->baseFeatureSet = BuildBaseFeatureSet(material, subMesh);
    drawSurf->shaderMaterialIndex = frameData->RegisterShaderMaterial(material);
    drawSurf->flags = flags;

    visCamera->visSurfs[visCamera->numVisSurfs++] = drawSurf;
}

void RenderWorld::SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial) {
    this->skyboxMaterial = skyboxMaterial;
}

void RenderWorld::SetupPyramidBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera) {
    if (renderCamera->GetBloomMipCount() < 2) {
        return;
    }

    visCamera->bloomDownTexture = renderCamera->GetBloomDownTexture();
    visCamera->bloomUpTexture = renderCamera->GetBloomUpTexture();
    visCamera->bloomMipCount = renderCamera->GetBloomMipCount();

    for (uint32_t mipLevel = 0; mipLevel < RenderBloomDownMaxLevelCount; ++mipLevel) {
        visCamera->bloomMipWidths[mipLevel] = renderCamera->GetBloomMipWidths()[mipLevel];
        visCamera->bloomMipHeights[mipLevel] = renderCamera->GetBloomMipHeights()[mipLevel];
        visCamera->bloomDownMipSRVs[mipLevel] = renderCamera->GetBloomDownMipSRVs()[mipLevel];
        visCamera->bloomDownMipUAVs[mipLevel] = renderCamera->GetBloomDownMipUAVs()[mipLevel];
    }
    for (uint32_t mipLevel = 0; mipLevel < RenderBloomUpMaxLevelCount; ++mipLevel) {
        visCamera->bloomUpMipSRVs[mipLevel] = renderCamera->GetBloomUpMipSRVs()[mipLevel];
        visCamera->bloomUpMipUAVs[mipLevel] = renderCamera->GetBloomUpMipUAVs()[mipLevel];
    }
}

void RenderWorld::SetupConvolutionBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera) {
    if (renderCamera->GetFFTBloomSize() == 0 ||
        !renderCamera->GetFFTBloomConvolutionOutputTexture() ||
        renderCamera->GetFFTBloomConvolutionOutputWidth() == 0 ||
        renderCamera->GetFFTBloomConvolutionOutputHeight() == 0) {
        return;
    }

    renderCamera->SetConvolutionBloomKernelRotation(r_convolutionBloomKernelRotation.GetFloat());
    renderCamera->SetConvolutionBloomKernelSizeScale(r_convolutionBloomKernelSizeScale.GetFloat());
    renderCamera->SetConvolutionBloomPaddingTrim(r_convolutionBloomPaddingTrim.GetFloat());

    const uint32_t fftBloomSize = renderCamera->GetFFTBloomSize();
    const bool updateFFTBloomKernelBuffer = renderCamera->IsFFTBloomKernelDirty();

    Texture *fftBloomKernelSourceTexture = nullptr;
    RHI::Texture *fftBloomKernelSourceRHITexture = nullptr;
    if (updateFFTBloomKernelBuffer) {
        fftBloomKernelSourceTexture = frameData->RetainTexture(renderCamera->GetFFTBloomKernelTexture());
        if (!fftBloomKernelSourceTexture || !fftBloomKernelSourceTexture->IsReadyForRendering()) {
            return;
        }
        fftBloomKernelSourceRHITexture = fftBloomKernelSourceTexture->GetRHITexture();

        renderCamera->UpdateConvolutionBloomGuardedContentRect();
    }

    visCamera->fftBloomSize = fftBloomSize;
    visCamera->fftBloomScreenSpatialBuffer = renderCamera->GetFFTBloomScreenSpatialBuffer();
    visCamera->fftBloomScreenSpectrumBuffer = renderCamera->GetFFTBloomScreenSpectrumBuffer();
    visCamera->fftBloomKernelSourceTexture = updateFFTBloomKernelBuffer ? fftBloomKernelSourceRHITexture : nullptr;
    visCamera->fftBloomKernelSpatialBuffer = renderCamera->GetFFTBloomKernelSpatialBuffer();
    visCamera->fftBloomKernelSpectrumBuffer = renderCamera->GetFFTBloomKernelSpectrumBuffer();
    visCamera->fftBloomWorkBuffer = renderCamera->GetFFTBloomWorkBuffer();
    visCamera->fftBloomConvolutionOutputTexture = renderCamera->GetFFTBloomConvolutionOutputTexture();
    visCamera->fftBloomConvolutionOutputWidth = renderCamera->GetFFTBloomConvolutionOutputWidth();
    visCamera->fftBloomConvolutionOutputHeight = renderCamera->GetFFTBloomConvolutionOutputHeight();
    visCamera->fftBloomKernelRotation = renderCamera->GetFFTBloomKernelRotation();
    visCamera->fftBloomKernelSizeScale = renderCamera->GetFFTBloomKernelSizeScale();
    visCamera->fftBloomPaddingTrim = renderCamera->GetFFTBloomPaddingTrim();
    visCamera->fftBloomKernelExtents = renderCamera->GetFFTBloomKernelExtents();
    visCamera->fftBloomGuardedContentRect = renderCamera->GetFFTBloomGuardedContentRect();
    visCamera->fftBloomKernelSpatialScale = renderCamera->GetFFTBloomKernelSpatialScale();
    visCamera->fftBloomKernelNormalizationFactor = renderCamera->GetFFTBloomKernelNormalizationFactor();
    visCamera->fftBloomKernelDirty = updateFFTBloomKernelBuffer;

    if (updateFFTBloomKernelBuffer) {
        renderCamera->MarkConvolutionBloomKernelClean();
    }
}

void RenderWorld::RenderScene(RenderCamera *renderCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene");

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();
    RenderContext *renderContext = RenderContext::activeContext;
    const BE1::Rect &outputRect = renderCamera->desc.outputRect;
    const uint32_t renderWidth = renderCamera->desc.renderWidth > 0 ? renderCamera->desc.renderWidth : (outputRect.w > 0 ? static_cast<uint32_t>(outputRect.w) : 0);
    const uint32_t renderHeight = renderCamera->desc.renderHeight > 0 ? renderCamera->desc.renderHeight : (outputRect.h > 0 ? static_cast<uint32_t>(outputRect.h) : 0);

    // RenderCamera 정보를 기반으로 프레임 데이터에 필요한 VisCamera 를 구성한다.
    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    renderCamera->EnsureRenderTextures(renderWidth, renderHeight, renderContext->GetMainRTColorFormat(), renderContext->GetMainRTDepthFormat(), renderContext->GetMainRTSampleCount(), renderContext->GetBaseClearColor());
    renderCamera->EnsurePostProcessResources();

    const RenderBloomMode bloomMode = static_cast<RenderBloomMode>(BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::Convolution)));
    const bool canUseHiZ = renderCamera->GetDepthTexture() != nullptr;
    const bool requestHiZOcclusionCulling = !BE1::HasFlag(renderCamera->desc.flags, RenderCameraFlag::DisableHiZCulling) && canUseHiZ && r_hiZOcclusionCulling.GetBool();
    const bool requestHiZDebugPreview = canUseHiZ && r_hiZDebugPreview.GetBool() && renderCamera->EnsureHiZDebugPreviewTexture(renderWidth, renderHeight);
    const bool requestHiZGeneration = requestHiZOcclusionCulling;

    visCamera->maxVisSurfs = VisCamera::MaxVisSurfs;
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->outputRect = outputRect;
    visCamera->renderWidth = renderWidth;
    visCamera->renderHeight = renderHeight;
    visCamera->clearMethod = renderCamera->desc.clearMethod;
    visCamera->clearColorMode = renderCamera->desc.clearColorMode;
    visCamera->clearColor = renderCamera->desc.clearColor;
    visCamera->projectionPolicy = renderCamera->desc.projectionPolicy;
    visCamera->flags = renderCamera->desc.flags;
    visCamera->rtColorTexture = renderCamera->GetRTColorTexture();
    visCamera->rtColorMSAATexture = renderCamera->GetRTColorMSAATexture();
    visCamera->depthTexture = renderCamera->GetDepthTexture();
    visCamera->rtNormalTexture = renderCamera->GetRTNormalTexture();
    visCamera->rtVelocityTexture = renderCamera->GetRTVelocityTexture();

    visCamera->generateHiZ = requestHiZGeneration;
    if (visCamera->generateHiZ) {
        visCamera->hiZTexture = renderCamera->GetHiZTexture();
        visCamera->hiZMipCount = renderCamera->GetHiZMipCount();

        for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
            visCamera->hiZMipSRVs[mipLevel] = renderCamera->GetHiZMipSRVs()[mipLevel];
            visCamera->hiZMipUAVs[mipLevel] = renderCamera->GetHiZMipUAVs()[mipLevel];
            visCamera->hiZMipWidths[mipLevel] = renderCamera->GetHiZMipWidths()[mipLevel];
            visCamera->hiZMipHeights[mipLevel] = renderCamera->GetHiZMipHeights()[mipLevel];
        }

        visCamera->hiZOcclusionCulling = renderCamera->HasPreviousHiZ() && requestHiZOcclusionCulling;
        if (visCamera->hiZOcclusionCulling) {
            visCamera->hiZCullVisibleInstanceBuffer = renderCamera->GetHiZCullVisibleInstanceBuffer();
            visCamera->hiZCullIndirectArgsBuffer = renderCamera->GetHiZCullIndirectArgsBuffer();
        }

        visCamera->hiZDebugPreview = requestHiZDebugPreview;
        if (visCamera->hiZDebugPreview) {
            visCamera->hiZDebugPreviewTexture = renderCamera->GetHiZDebugPreviewTexture();
            visCamera->hiZDebugProjectionDepthA = renderCamera->projMatrix[2][2];
            visCamera->hiZDebugProjectionDepthB = renderCamera->projMatrix[2][3];
            visCamera->hiZDebugNDCDepthScale = renderCamera->desc.projectionPolicy.IsZNegativeOneToOne() ? 2.0f : 1.0f;
            visCamera->hiZDebugNDCDepthBias = renderCamera->desc.projectionPolicy.IsZNegativeOneToOne() ? -1.0f : 0.0f;
            visCamera->hiZDebugLinearDepthScale = renderCamera->desc.zFar > 0.0f ? 1.0f / renderCamera->desc.zFar : 1.0f;
            visCamera->hiZDebugOrthogonal = renderCamera->desc.orthogonal ? 1u : 0u;
        }
    }

    visCamera->postProcessColorTexture = renderCamera->GetPostProcessColorTexture();
    visCamera->luminanceBuffer = renderCamera->GetLuminanceBuffer();

    if (bloomMode == RenderBloomMode::Pyramid) {
        SetupPyramidBloom(frameData, renderCamera, visCamera);
    } else if (bloomMode == RenderBloomMode::Convolution) {
        SetupConvolutionBloom(frameData, renderCamera, visCamera);
    }

    visCamera->worldAABB.Clear();
    visCamera->is2D = false;
    visCamera->viewMatrix = renderCamera->viewMatrix;
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;
    visCamera->invViewProjMatrix = renderCamera->invViewProjMatrix;

    const bool temporalHistoryEnabled = !BE1::HasFlag(renderCamera->desc.flags, RenderCameraFlag::DisableTemporalHistory);
    const bool usePreviousMatrix = temporalHistoryEnabled && renderCamera->previousMatrixValid && !renderCamera->temporalHistoryInvalidated;

    if (usePreviousMatrix) {
        visCamera->prevViewProjMatrix = renderCamera->previousViewProjMatrix;
        visCamera->prevInvViewProjMatrix = renderCamera->previousInvViewProjMatrix;
    } else {
        visCamera->prevViewProjMatrix = renderCamera->viewProjMatrix;
        visCamera->prevInvViewProjMatrix = renderCamera->invViewProjMatrix;
    }

    if (!r_hiZOcclusionCullingFreeze.GetBool() || !renderCamera->hasPreviousHiZ) {
        renderCamera->hiZCullViewProjMatrix = visCamera->prevViewProjMatrix;
    }
    visCamera->hiZCullViewProjMatrix = renderCamera->hiZCullViewProjMatrix;

    DrawCamera(frameData, renderCamera, visCamera);

    if (temporalHistoryEnabled) {
        renderCamera->previousViewProjMatrix = renderCamera->viewProjMatrix;
        renderCamera->previousInvViewProjMatrix = renderCamera->invViewProjMatrix;
        renderCamera->previousMatrixValid = true;
        renderCamera->temporalHistoryInvalidated = false;
    }

    if (!r_hiZOcclusionCullingFreeze.GetBool()) {
        renderCamera->hasPreviousHiZ = visCamera->generateHiZ;
    }
}

void RenderWorld::DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

#ifdef USE_DBVT
    UpdatePendingObjectProxies();
    UpdateSleepingObjectProxies();
#endif

    // objectDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 RenderObject 들을 찾아낸다.
    // 보이는 RenderObject 들로 프레임 데이터에 VisObject 들을 등록한다.
    FindVisObjects(frameData, renderCamera, visCamera);

    // 보이는 VisObject 에서 mesh surface 들을 visCamera->visSurfs 에 추가한다.
    AddMeshes(frameData, renderCamera, visCamera);

    // 보이는 visObjects 중에 raw mesh 를 visCamera->visSurfs 에 추가한다.
    //AddRawMeshes(frameData, renderCamera, visCamera);

    // 보이는 visObjects 중에 text mesh 를 visCamera->visSurfs 에 추가한다.
    AddTextMeshes(frameData, renderCamera, visCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frameData, renderCamera, visCamera);

    // 렌더 패스를 구성한다.
    renderPassBuilder.BuildMain(frameData, visCamera);

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}

VisObject *RenderWorld::RegisterVisObject(RenderFrameData *frameData, RenderObject *renderObject) {
    assert(frameActive);

    if (IsVisObjectRegistered(renderObject)) {
        return nullptr;
    }

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return nullptr;
    }
    renderObject->visObject = visObject;
    renderObject->viewCount = viewCount;

    if (renderObject->desc.meshInstance) {
        visObject->meshInstance = frameData->RetainMeshInstance(renderObject->desc.meshInstance);
    }
    const int numMaterials = renderObject->desc.materials.Count();
    visObject->numMaterials = static_cast<uint32_t>(numMaterials);
    if (numMaterials > 0) {
        visObject->materials = (const Material **)frameData->MemAlloc(numMaterials * sizeof(visObject->materials[0]));
        for (int i = 0; i < numMaterials; ++i) {
            visObject->materials[i] = frameData->RetainMaterial(renderObject->desc.materials[i]);
        }
    }
    if (renderObject->desc.numVerts > 0 && renderObject->desc.numIndexes > 0) {
        visObject->numVerts = renderObject->desc.numVerts;
        visObject->verts = renderObject->desc.verts;
        visObject->numIndexes = renderObject->desc.numIndexes;
        visObject->indexes = renderObject->desc.indexes;
    }
    if (renderObject->desc.text) {
        RenderObject::TextDesc &textDesc = *(renderObject->desc.text);
        if (textDesc.font) {
            visObject->font = frameData->RetainFont(textDesc.font);
        }
        if (!textDesc.text.IsEmpty()) {
            int size = textDesc.text.Length() + 1;
            char *textMemPtr = (char *)frameData->MemAlloc(size);
            BE1::Str::Copynz(textMemPtr, textDesc.text.c_str(), size);
            visObject->text = textMemPtr;
            visObject->textRect = textDesc.textRect;
            visObject->textShadowOffset = textDesc.textShadowOffset;
            visObject->textShadowColor = textDesc.textShadowColor;
            visObject->textParams = textDesc.textParams;
            visObject->textScale = textDesc.textScale;
            visObject->textLineSpacing = textDesc.textLineSpacing;
        }
    }

    return visObject;
}

void RenderWorld::SetVisObjectWorldMatrices(const RenderObject *renderObject, VisObject *visObject) {
    visObject->worldMatrix = renderObject->GetWorldMatrix();

    // 바로 직전 프레임에 prevWorldMatrix 를 세팅했었다면, 움직이던 오브젝트이므로 prevWorldMatrix 를 그대로 사용한다.
    const bool hasContinuousPrevWorldMatrix = renderWorldFrameIndex > 0 &&
        renderObject->prevWorldMatrixValid &&
        renderObject->prevWorldMatrixFrameIndex == renderWorldFrameIndex - 1;

    visObject->prevWorldMatrix = hasContinuousPrevWorldMatrix ? renderObject->prevWorldMatrix : renderObject->desc.worldMatrix;
}

void RenderWorld::FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects");

    const uint32_t startVisibleObjectCount = frameData->NumVisObjects();

    visCamera->worldAABB.Clear();

#ifdef USE_DBVT
    // Frustum 에 교차된 RenderObject 들을 추려내서 프레임 데이터에 VisObject 로 등록한다.
    // 쿼리 콜백 함수의 리턴값이 true 라면, 다음 쿼리를 진행한다.
    auto addVisibleObjects = [this, frameData, renderCamera, visCamera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;
        if (!renderObject) {
            return true;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            return true;
        }

        renderObject->visibleFrameIndex = renderWorldFrameIndex;

        SetVisObjectWorldMatrices(renderObject, visObject);
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(proxy->worldAABB);
        return true;
    };

    objectDbvt.QueryFrustum(renderCamera->frustum, addVisibleObjects);

    for (int objectIndex : dynamicObjectIndices) {
        RenderObject *renderObject = renderObjects[objectIndex];

        if (!r_skipFrustumCulling.GetBool()) {
            if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
                continue;
            }
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        renderObject->visibleFrameIndex = renderWorldFrameIndex;

        SetVisObjectWorldMatrices(renderObject, visObject);
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#else
    // 등록된 모든 RenderObject 들을 개별 Culling 해서 프레임 데이터에 VisObject 로 등록한다.
    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject) {
            continue;
        }

        if (!r_skipFrustumCulling.GetBool()) {
            if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
                continue;
            }
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        renderObject->visibleFrameIndex = renderWorldFrameIndex;

        SetVisObjectWorldMatrices(renderObject, visObject);
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#endif

    frameData->AddObjectStats(static_cast<uint32_t>(renderObjects.Count()), frameData->NumVisObjects() - startVisibleObjectCount);
}

void RenderWorld::UpdateSkinningJointCaches(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints) {
    if (numVisObjects == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), numTasks);

    if (numTasks > 1) {
        UpdateSkinningJointCachesWithTask(frameData, visObjects, numVisObjects, numSkinningJoints, numTasks);
    } else {
        UpdateSkinningJointCachesWithoutTask(frameData, visObjects, numVisObjects);
    }
#else
    UpdateSkinningJointCachesWithoutTask(frameData, visObjects, numVisObjects);
#endif
}

void RenderWorld::UpdateSkinningJointCachesWithoutTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithoutTask");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    for (uint32_t visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        VisObject *visObject = visObjects[visObjectIndex];
        visObject->meshInstance->UpdateSkinningJointCache(frameThreadData, visObject->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTask(RenderWorld::SkinningJointCacheTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesByTask");

    RHI::FrameThreadData *frameThreadData = taskDesc->frameData->GetThreadData(taskDesc->threadIndex);

    for (uint32_t visObjectIndex = 0; visObjectIndex < taskDesc->numVisObjects; ++visObjectIndex) {
        VisObject *visObject = taskDesc->visObjects[visObjectIndex];
        visObject->meshInstance->UpdateSkinningJointCache(frameThreadData, visObject->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTaskFunction(void *data) {
    RenderWorld::SkinningJointCacheTaskDesc *taskDesc = reinterpret_cast<RenderWorld::SkinningJointCacheTaskDesc *>(data);
    taskDesc->renderWorld->UpdateSkinningJointCachesByTask(taskDesc);
}

void RenderWorld::UpdateSkinningJointCachesWithTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithTask");

    uint32_t targetJointsPerTask = (numSkinningJoints + numTasks - 1) / numTasks;
    uint32_t startIndex = 0;

    skinningJointCacheTaskDescs.Reserve(numTasks);
    skinningJointCacheTaskDescs.SetCount(0, false);

    while (startIndex < numVisObjects && skinningJointCacheTaskDescs.Count() + 1 < (int)numTasks) {
        uint32_t endIndex = startIndex;
        uint32_t numTaskJoints = 0;

        do {
            VisObject *visObject = visObjects[endIndex];
            numTaskJoints += BE1::Max(1, visObject->meshInstance->NumSkinningMatrices());
            endIndex++;
        } while (endIndex < numVisObjects && numTaskJoints < targetJointsPerTask);

        SkinningJointCacheTaskDesc &taskDesc = skinningJointCacheTaskDescs.Alloc();
        taskDesc.threadIndex = skinningJointCacheTaskDescs.Count() - 1;
        taskDesc.renderWorld = this;
        taskDesc.frameData = frameData;
        taskDesc.visObjects = &visObjects[startIndex];
        taskDesc.numVisObjects = endIndex - startIndex;

        startIndex = endIndex;
    }

    if (startIndex < numVisObjects) {
        SkinningJointCacheTaskDesc &taskDesc = skinningJointCacheTaskDescs.Alloc();
        taskDesc.threadIndex = skinningJointCacheTaskDescs.Count() - 1;
        taskDesc.renderWorld = this;
        taskDesc.frameData = frameData;
        taskDesc.visObjects = &visObjects[startIndex];
        taskDesc.numVisObjects = numVisObjects - startIndex;
    }

    for (int taskIndex = 0; taskIndex < skinningJointCacheTaskDescs.Count(); ++taskIndex) {
        BE1::Engine::taskManager->AddTask(RenderWorld::UpdateSkinningJointCachesByTaskFunction, &skinningJointCacheTaskDescs[taskIndex], renderWorldGroupId, false);
    }

    BE1::Engine::taskManager->WaitFinish(renderWorldGroupId, true);
}

void RenderWorld::UpdateVisibleSkinningJointCaches(RenderFrameData *frameData) {
    const uint32_t numVisObjects = frameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    uint32_t numSkinnedVisObjects = 0;
    uint32_t numSkinningJoints = 0;
    VisObject **skinnedVisObjects = (VisObject **)_alloca(numVisObjects * sizeof(skinnedVisObjects[0]));

    for (uint32_t visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        VisObject *visObject = frameData->GetVisObjects() + visObjectIndex;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->skeleton) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }
        if (!visObject->meshInstance->EnsureCurrent()) {
            continue;
        }

        skinnedVisObjects[numSkinnedVisObjects++] = visObject;
        numSkinningJoints += visObject->meshInstance->NumSkinningMatrices();
    }

    UpdateSkinningJointCaches(frameData, skinnedVisObjects, numSkinnedVisObjects, numSkinningJoints);
}

void RenderWorld::AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    UpdateVisibleSkinningJointCaches(frameData);

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }
        if (!visObject->meshInstance->EnsureCurrent()) {
            continue;
        }

        VisSurfFlag flags = VisSurfFlag::CameraVisible;
        const int numSurfaces = visObject->meshInstance->NumSurfaces();
        const bool cullSurfaces = !r_skipFrustumCulling.GetBool() && numSurfaces > 1;

        for (int meshSurfIndex = 0; meshSurfIndex < numSurfaces; ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = visObject->meshInstance->GetSurface(meshSurfIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                continue;
            }

            if (cullSurfaces) {
                const BE1::AABB &localAABB = meshSurf->subMesh->GetBoundingBox();
                if (!localAABB.IsCleared()) {
                    BE1::AABB worldAABB;
                    worldAABB.SetFromTransformedAABBFast(localAABB, visObject->worldMatrix);

                    if (renderCamera->frustum.CullAABB(worldAABB)) {
                        continue;
                    }
                }
            }
            AddVisSurf(frameData, visCamera, nullptr, visObject, GetVisObjectMaterial(visObject, meshSurf->materialIndex), meshSurf->subMesh, flags);
        }
    }
}

void RenderWorld::AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->text) {
            continue;
        }

        textMesh.Clear();

        if (visObject->textRect.IsEmpty()) {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visObject->textParams.bits.textDrawMode,
                (RenderObject::TextAnchor)visObject->textParams.bits.textAnchor, (RenderObject::TextHorzAlignment)visObject->textParams.bits.textHorzAlignment,
                visObject->textLineSpacing, visObject->textScale, BE1::Color4::white.ToUInt32(), visObject->textShadowColor, visObject->textShadowOffset, visObject->font, visObject->text);
        } else {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visObject->textParams.bits.textDrawMode, visObject->textRect,
                (RenderObject::TextHorzAlignment)visObject->textParams.bits.textHorzAlignment, (RenderObject::TextVertAlignment)visObject->textParams.bits.textVertAlignment,
                (RenderObject::TextHorzOverflow)visObject->textParams.bits.textHorzOverflow, (RenderObject::TextVertOverflow)visObject->textParams.bits.textVertOverflow,
                visObject->textLineSpacing, visObject->textScale, BE1::Color4::white.ToUInt32(), visObject->textShadowColor, visObject->textShadowOffset, visObject->font, visObject->text);
        }

        textMesh.CacheIndexes(frameData->GetThreadData(0));

        const int numSurfaces = textMesh.NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const GuiMesh::Surface *guiSurf = textMesh.GetSurface(surfaceIndex);
            if (!guiSurf->numIndexes) {
                break;
            }

            SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->numIndexes = guiSurf->numIndexes;
            subMesh->numVerts = guiSurf->numVerts;
            subMesh->vertexBuffer = guiSurf->vertexBuffer;
            subMesh->indexBuffer = guiSurf->indexBuffer;

            AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None);
        }
    }
}

void RenderWorld::AddRawMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddRawMeshes");

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (visObject->numVerts <= 0 || visObject->numIndexes <= 0) {
            continue;
        }

        RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
        RHI::VertexBuffer *vertexBuffer = frameThreadData->AllocVertex(sizeof(visObject->verts[0]), visObject->numVerts, visObject->verts);
        RHI::IndexBuffer *indexBuffer = frameThreadData->AllocIndex(sizeof(visObject->indexes[0]), visObject->numIndexes, visObject->indexes);
        if (!vertexBuffer || !indexBuffer) {
            continue;
        }

        // Copy this SubMesh to the temporary frame data to use in the backend.
        SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numVerts = visObject->numVerts;
        subMesh->numIndexes = visObject->numIndexes;
        subMesh->vertexBuffer = vertexBuffer;
        subMesh->indexBuffer = indexBuffer;

        AddVisSurf(frameData, visCamera, nullptr, visObject, GetVisObjectMaterial(visObject, 0), subMesh, VisSurfFlag::None);
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddSkyBoxMeshes");

    if (renderCamera->GetDesc().clearMethod != RenderCameraClearMethod::Skybox) {
        return;
    }
    if (!skyboxMaterial || !meshManager.defaultBoxMesh || !meshManager.defaultBoxMesh->IsReady()) {
        return;
    }

    const Mesh::Surface *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    if (!meshSurf || !meshSurf->subMesh) {
        return;
    }

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetTRS(renderCamera->GetDesc().origin, BE1::Mat3::identity, BE1::Vec3(10, 10, 10));
    visObject->prevWorldMatrix = visObject->worldMatrix;
    visObject->cameraVisible = true;
    Material *material = frameData->RetainMaterial(skyboxMaterial);

    AddVisSurf(frameData, visCamera, nullptr, visObject, material, meshSurf->subMesh, VisSurfFlag::CameraVisible | VisSurfFlag::SkipSelection | VisSurfFlag::Skybox);
}

void RenderWorld::RenderGUI(GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderGUI");

    assert(BE1::Engine::IsInMainThread());

    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();

    uint32_t screenWidth = RenderContext::activeContext->GetWidth();
    uint32_t screenHeight = RenderContext::activeContext->GetHeight();

    // guiMesh 를 기반으로 GUI VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frameData, screenWidth, screenHeight, guiMesh);
}

void RenderWorld::DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera");

    VisCamera *visCamera = frameData->AllocVisCamera();
    visCamera->maxVisSurfs = guiMesh.NumSurfaces();
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->outputRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    visCamera->renderWidth = screenWidth;
    visCamera->renderHeight = screenHeight;
    visCamera->clearMethod = RenderCameraClearMethod::NoClear;
    visCamera->projectionPolicy = RHI::renderer->GetProjectionPolicy();
    visCamera->flags = RenderCameraFlag::SkipPostProcess;
    //visCamera->time = BE1::PlatformTime::Milliseconds();
    //visCamera->orthogonal = true;
    //visCamera->zNear = 0.0f;
    //visCamera->zFar = 1.0f;
    visCamera->viewMatrix = BE1::Mat4::identity;
    visCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true); // Y 를 뒤집는다.
    visCamera->invViewProjMatrix = visCamera->viewProjMatrix.Inverse();
    visCamera->prevViewProjMatrix = visCamera->viewProjMatrix;
    visCamera->prevInvViewProjMatrix = visCamera->invViewProjMatrix;
    visCamera->hiZCullViewProjMatrix = visCamera->viewProjMatrix;
    visCamera->worldAABB.Clear();
    visCamera->is2D = true;

    VisObject *visObject = frameData->AllocVisObject();
    visObject->worldMatrix.SetIdentity();
    visObject->prevWorldMatrix.SetIdentity();

    guiMesh.CacheIndexes(frameData->GetThreadData(0));

    const int numSurfaces = guiMesh.NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
        subMesh->vertexBuffer = guiSurf->vertexBuffer;
        subMesh->indexBuffer = guiSurf->indexBuffer;

        AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGUI(frameData, visCamera);

    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}
