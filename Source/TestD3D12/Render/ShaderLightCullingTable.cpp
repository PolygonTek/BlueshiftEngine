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
#include "ShaderLightCullingTable.h"
#include "RenderFrameData.h"
#include "RenderInternal.h"

bool ShaderLightCullingTable::Init(uint32_t initialLightRefCapacity, uint32_t initialZBinCapacity, uint32_t initialLightTileMaskCapacity) {
    if (!EnsureBufferCapacity(initialLightRefCapacity, initialZBinCapacity, initialLightTileMaskCapacity)) {
        Shutdown();
        return false;
    }

    return true;
}

void ShaderLightCullingTable::Shutdown() {
    DestroyBuffer(lightRefBuffer);
    DestroyBuffer(lightZBinBuffer);
    DestroyBuffer(lightTileWordBuffer);
}

bool ShaderLightCullingTable::EnsureBufferCapacity(uint32_t lightRefCount, uint32_t numLightCullingZBins, uint32_t numLightCullingWords) {
    if (!EnsureBufferCapacity(lightRefBuffer, lightRefCount, sizeof(ShaderLightRef), RHI::Format::Unknown, sizeof(ShaderLightRef), false, "shaderLightRefTableBuffer")) {
        return false;
    }

    if (!EnsureBufferCapacity(lightZBinBuffer, numLightCullingZBins, sizeof(uint32_t), RHI::Format::R32_UINT, 0, false, "shaderLightZBinTableBuffer")) {
        return false;
    }

    if (!EnsureBufferCapacity(lightTileWordBuffer, numLightCullingWords, sizeof(uint32_t), RHI::Format::R32_UINT, 0, true, "shaderLightTileWordTableBuffer")) {
        return false;
    }

    return true;
}

bool ShaderLightCullingTable::EnsureBufferCapacity(TableBuffer &tableBuffer, uint32_t count, uint32_t elementSize, RHI::Format format, uint32_t stride, bool unorderedAccess, const char *name) {
    if (count <= tableBuffer.capacity) {
        return true;
    }

    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 1;
    while (newCapacity < count) {
        if (newCapacity > UINT32_MAX / 2) {
            return false;
        }
        newCapacity <<= 1;
    }

    if (newCapacity > UINT32_MAX / elementSize) {
        return false;
    }

    DestroyBuffer(tableBuffer);

    const uint32_t bufferSize = newCapacity * elementSize;

    // GPU 디폴트 버퍼를 생성한다.
    const RHI::BufferFlag bufferFlags = unorderedAccess ? RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess : RHI::BufferFlag::ShaderResource;
    tableBuffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, bufferFlags, bufferSize, nullptr);
    if (!tableBuffer.buffer) {
        DestroyBuffer(tableBuffer);
        return false;
    }

    RHI::renderer->SetName(tableBuffer.buffer, name);

    tableBuffer.view = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::SRV, format, stride, 0, bufferSize);
    if (!tableBuffer.view.IsValid()) {
        DestroyBuffer(tableBuffer);
        return false;
    }

    if (unorderedAccess) {
        tableBuffer.uavView = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::UAV, format, stride, 0, bufferSize);
        if (!tableBuffer.uavView.IsValid()) {
            DestroyBuffer(tableBuffer);
            return false;
        }
    }

    tableBuffer.capacity = newCapacity;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.bindlessIndex = tableBuffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, tableBuffer.view);
    return true;
}

bool ShaderLightCullingTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrameData *frameData, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderLightCullingTable::Upload");

    const uint32_t lightRefCount = frameData->NumVisLocalLights();
    const uint32_t numLightCullingZBins = frameData->NumLightCullingZBins();
    const uint32_t numLightCullingWords = frameData->NumLightCullingWords();

    if (lightRefCount == 0 && numLightCullingZBins == 0 && numLightCullingWords == 0) {
        return true;
    }

    // NOTE: lightTileWordBuffer 는 업로드 대상은 아니지만 여기서 EnsureBufferCapacity 는 해준다.
    if (lightRefCount > lightRefBuffer.capacity || numLightCullingZBins > lightZBinBuffer.capacity || numLightCullingWords > lightTileWordBuffer.capacity) {
        assert(allowResize);
        if (!allowResize || !EnsureBufferCapacity(lightRefCount, numLightCullingZBins, numLightCullingWords)) {
            return false;
        }
    }

    RHI::Buffer *lightRefUploadBuffer = nullptr;
    ShaderLightRef *shaderLightRefs = nullptr;

    if (lightRefCount > 0) {
        lightRefUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLightRef), lightRefCount, nullptr);
        if (!lightRefUploadBuffer || !lightRefUploadBuffer->mappedPtr) {
            return false;
        }
        shaderLightRefs = reinterpret_cast<ShaderLightRef *>(lightRefUploadBuffer->mappedPtr);
    }

    RHI::Buffer *zBinUploadBuffer = nullptr;
    uint32_t *shaderLightZBins = nullptr;

    if (numLightCullingZBins > 0) {
        zBinUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(uint32_t), numLightCullingZBins, nullptr);
        if (!zBinUploadBuffer || !zBinUploadBuffer->mappedPtr) {
            return false;
        }
        shaderLightZBins = reinterpret_cast<uint32_t *>(zBinUploadBuffer->mappedPtr);
    }

    const VisCamera *visCameras = frameData->GetVisCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];

        if (visCamera.numVisLocalLights > 0) {
            if (visCamera.lightCulling.lightRefTableOffset > lightRefCount || visCamera.numVisLocalLights > lightRefCount - visCamera.lightCulling.lightRefTableOffset) {
                assert(0);
                return false;
            }

            for (uint32_t lightIndex = 0; lightIndex < visCamera.numVisLocalLights; ++lightIndex) {
                BuildLightRef(visCamera.visLocalLights[lightIndex], shaderLightRefs[visCamera.lightCulling.lightRefTableOffset + lightIndex]);
            }
        }

        if (visCamera.lightCulling.zBins) {
            if (visCamera.lightCulling.zBinTableOffset > numLightCullingZBins || ShaderLightZBinCount > numLightCullingZBins - visCamera.lightCulling.zBinTableOffset) {
                assert(0);
                return false;
            }

            BE1::simdProcessor->Memcpy(shaderLightZBins + visCamera.lightCulling.zBinTableOffset, visCamera.lightCulling.zBins, ShaderLightZBinCount * sizeof(uint32_t));
        }
    }

    UploadBuffer(commandList, lightRefBuffer, lightRefUploadBuffer, lightRefCount * sizeof(ShaderLightRef));
    UploadBuffer(commandList, lightZBinBuffer, zBinUploadBuffer, numLightCullingZBins * sizeof(uint32_t));
    return true;
}

bool ShaderLightCullingTable::DispatchLightCulling(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::PipelineState *pipelineState, const RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("ShaderLightCullingTable::DispatchLightCulling");

    const uint32_t numLightCullingWords = frameData->NumLightCullingWords();
    if (numLightCullingWords == 0) {
        return true;
    }

    if (!pipelineState || frameData->NumVisLocalLights() > lightRefBuffer.capacity || numLightCullingWords > lightTileWordBuffer.capacity) {
        return false;
    }

    const VisCamera *visCameras = frameData->GetVisCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];
        if (visCamera.lightCulling.wordsPerTile == 0) {
            continue;
        }

        const uint32_t cameraTileWordCount = visCamera.lightCulling.tileCountX * visCamera.lightCulling.tileCountY * visCamera.lightCulling.wordsPerTile;
        if (visCamera.lightCulling.lightRefTableOffset > frameData->NumVisLocalLights() ||
            visCamera.numVisLocalLights > frameData->NumVisLocalLights() - visCamera.lightCulling.lightRefTableOffset ||
            visCamera.lightCulling.tileWordTableOffset > numLightCullingWords ||
            cameraTileWordCount > numLightCullingWords - visCamera.lightCulling.tileWordTableOffset) {
            assert(0);
            return false;
        }
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lightRefBuffer.buffer, lightRefBuffer.state, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(lightTileWordBuffer.buffer, lightTileWordBuffer.state, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    lightRefBuffer.state = RHI::GPUResourceState::ShaderResourceCompute;
    lightTileWordBuffer.state = RHI::GPUResourceState::UnorderedAccess;

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetBuffer(commandList, 0, false, lightRefBuffer.buffer, lightRefBuffer.view);
    RHI::renderer->SetBuffer(commandList, 0, true, lightTileWordBuffer.buffer, lightTileWordBuffer.uavView);

    // 전체 카메라에 대해서 라이트 컬링 컴퓨트 셰이더를 실행한다.
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];
        if (visCamera.lightCulling.wordsPerTile == 0) {
            continue;
        }

        RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(ShaderLightCullingParams));
        if (!constantBuffer || !constantBuffer->mappedPtr) {
            return false;
        }

        ShaderLightCullingParams *params = reinterpret_cast<ShaderLightCullingParams *>(constantBuffer->mappedPtr);
        params->numLocalLights = visCamera.numVisLocalLights;
        params->lightRefTableOffset = visCamera.lightCulling.lightRefTableOffset;
        params->tileWordTableOffset = visCamera.lightCulling.tileWordTableOffset;
        params->tileCountX = visCamera.lightCulling.tileCountX;
        params->tileCountY = visCamera.lightCulling.tileCountY;
        params->wordsPerTile = visCamera.lightCulling.wordsPerTile;
        params->padding0 = 0;
        params->padding1 = 0;

        RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

        // XY 축은 한 스레드 그룹이 몇개의 타일을 계산할 것인지를 기준으로 스레드 그룹 개수를 계산.
        // Z 축은 한 스레드 그룹이 하나의 mask word 를 처리하는 것을 기준으로 스레드 그룹 개수를 계산.
        const uint32_t threadGroupCountX = BE1::DivCeil(visCamera.lightCulling.tileCountX, ShaderLightCullingThreadGroupSizeX);
        const uint32_t threadGroupCountY = BE1::DivCeil(visCamera.lightCulling.tileCountY, ShaderLightCullingThreadGroupSizeY);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, visCamera.lightCulling.wordsPerTile);
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lightRefBuffer.buffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(lightTileWordBuffer.buffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    lightRefBuffer.state = RHI::GPUResourceState::ShaderResource;
    lightTileWordBuffer.state = RHI::GPUResourceState::ShaderResource;
    return true;
}

void ShaderLightCullingTable::UploadBuffer(RHI::CommandList *commandList, TableBuffer &tableBuffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize) {
    if (uploadSize == 0) {
        return;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    tableBuffer.state = RHI::GPUResourceState::ShaderResource;
}

void ShaderLightCullingTable::DestroyBuffer(TableBuffer &tableBuffer) {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
        tableBuffer.buffer = nullptr;
    }

    tableBuffer.view = RHI::ViewID::InvalidID;
    tableBuffer.uavView = RHI::ViewID::InvalidID;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.capacity = 0;
    tableBuffer.bindlessIndex = 0;
}

void ShaderLightCullingTable::BuildLightRef(const VisLight *visLight, ShaderLightRef &outShaderLightRef) const {
    const uint32_t lightType = static_cast<uint32_t>(visLight->type);
    assert(lightType <= ShaderLightTypeMask);

    // 하위 비트: 라이트 타입
    // 상위 비트: 라이트 타입 별 인덱스
    outShaderLightRef.packedLightRef = (visLight->typeIndex << ShaderLightTypeBitCount) | lightType;
    outShaderLightRef.packedTileMin = visLight->packedTileMin;
    outShaderLightRef.packedTileMax = visLight->packedTileMax;
}
