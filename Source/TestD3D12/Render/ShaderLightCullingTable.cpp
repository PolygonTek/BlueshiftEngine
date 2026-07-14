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
    DestroyBuffer(lightTileMaskBuffer);
}

bool ShaderLightCullingTable::EnsureBufferCapacity(uint32_t lightRefCount, uint32_t lightZBinCount, uint32_t lightTileMaskCount) {
    if (!EnsureBufferCapacity(lightRefBuffer, lightRefCount, sizeof(ShaderLightRef), RHI::Format::Unknown, sizeof(ShaderLightRef), false, "shaderLightRefTableBuffer")) {
        return false;
    }

    if (!EnsureBufferCapacity(lightZBinBuffer, lightZBinCount, sizeof(uint32_t), RHI::Format::R32_UINT, 0, false, "shaderLightZBinTableBuffer")) {
        return false;
    }

    if (!EnsureBufferCapacity(lightTileMaskBuffer, lightTileMaskCount, sizeof(uint32_t), RHI::Format::R32_UINT, 0, true, "shaderLightTileMaskTableBuffer")) {
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

    const uint32_t lightRefCount = frameData->NumVisLights();
    const uint32_t lightZBinCount = frameData->GetLightZBinCount();
    const uint32_t lightTileMaskCount = frameData->GetLightTileMaskCount();

    if (lightRefCount == 0 && lightZBinCount == 0 && lightTileMaskCount == 0) {
        return true;
    }

    // NOTE: lightTileMaskBuffer 는 업로드 대상은 아니지만 여기서 EnsureBufferCapacity 는 해준다.
    if (lightRefCount > lightRefBuffer.capacity || lightZBinCount > lightZBinBuffer.capacity || lightTileMaskCount > lightTileMaskBuffer.capacity) {
        assert(allowResize);
        if (!allowResize || !EnsureBufferCapacity(lightRefCount, lightZBinCount, lightTileMaskCount)) {
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

    if (lightZBinCount > 0) {
        zBinUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(uint32_t), lightZBinCount, nullptr);
        if (!zBinUploadBuffer || !zBinUploadBuffer->mappedPtr) {
            return false;
        }
        shaderLightZBins = reinterpret_cast<uint32_t *>(zBinUploadBuffer->mappedPtr);
    }

    const VisCamera *visCameras = frameData->GetVisCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];

        if (visCamera.numVisLights > 0) {
            if (visCamera.shaderLightRefOffset > lightRefCount ||
                visCamera.numVisLights > lightRefCount - visCamera.shaderLightRefOffset) {
                assert(0);
                return false;
            }

            for (uint32_t lightIndex = 0; lightIndex < visCamera.numVisLights; ++lightIndex) {
                Build(visCamera, *visCamera.visLights[lightIndex], shaderLightRefs[visCamera.shaderLightRefOffset + lightIndex]);
            }
        }

        if (visCamera.lightZBins) {
            if (visCamera.shaderLightZBinOffset > lightZBinCount ||
                ShaderLightZBinCount > lightZBinCount - visCamera.shaderLightZBinOffset) {
                assert(0);
                return false;
            }

            BE1::simdProcessor->Memcpy(shaderLightZBins + visCamera.shaderLightZBinOffset, visCamera.lightZBins, ShaderLightZBinCount * sizeof(uint32_t));
        }
    }

    UploadBuffer(commandList, lightRefBuffer, lightRefUploadBuffer, lightRefCount * sizeof(ShaderLightRef));
    UploadBuffer(commandList, lightZBinBuffer, zBinUploadBuffer, lightZBinCount * sizeof(uint32_t));
    return true;
}

bool ShaderLightCullingTable::DispatchLightCulling(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::PipelineState *pipelineState, const RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("ShaderLightCullingTable::DispatchLightCulling");

    const uint32_t lightTileMaskCount = frameData->GetLightTileMaskCount();
    if (lightTileMaskCount == 0) {
        return true;
    }

    if (!pipelineState || frameData->NumVisLights() > lightRefBuffer.capacity || lightTileMaskCount > lightTileMaskBuffer.capacity) {
        return false;
    }

    const VisCamera *visCameras = frameData->GetVisCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];
        if (visCamera.lightMaskWordCount == 0) {
            continue;
        }

        const uint32_t cameraMaskCount = visCamera.lightTileCountX * visCamera.lightTileCountY * visCamera.lightMaskWordCount;
        if (visCamera.shaderLightRefOffset > frameData->NumVisLights() ||
            visCamera.numVisLights > frameData->NumVisLights() - visCamera.shaderLightRefOffset ||
            visCamera.shaderLightTileMaskOffset > lightTileMaskCount ||
            cameraMaskCount > lightTileMaskCount - visCamera.shaderLightTileMaskOffset) {
            assert(0);
            return false;
        }
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lightRefBuffer.buffer, lightRefBuffer.state, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(lightTileMaskBuffer.buffer, lightTileMaskBuffer.state, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    lightRefBuffer.state = RHI::GPUResourceState::ShaderResourceCompute;
    lightTileMaskBuffer.state = RHI::GPUResourceState::UnorderedAccess;

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetBuffer(commandList, 0, false, lightRefBuffer.buffer, lightRefBuffer.view);
    RHI::renderer->SetBuffer(commandList, 0, true, lightTileMaskBuffer.buffer, lightTileMaskBuffer.uavView);

    // 전체 카메라에 대해서 라이트 컬링 컴퓨트 셰이더를 실행한다.
    for (uint32_t cameraIndex = 0; cameraIndex < frameData->NumVisCameras(); ++cameraIndex) {
        const VisCamera &visCamera = visCameras[cameraIndex];
        if (visCamera.lightMaskWordCount == 0) {
            continue;
        }

        RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(ShaderLightCullingParams));
        if (!constantBuffer || !constantBuffer->mappedPtr) {
            return false;
        }

        ShaderLightCullingParams *params = reinterpret_cast<ShaderLightCullingParams *>(constantBuffer->mappedPtr);
        params->numLights = visCamera.numVisLights;
        params->lightRefOffset = visCamera.shaderLightRefOffset;
        params->lightTileMaskOffset = visCamera.shaderLightTileMaskOffset;
        params->lightTileCountX = visCamera.lightTileCountX;
        params->lightTileCountY = visCamera.lightTileCountY;
        params->lightMaskWordCount = visCamera.lightMaskWordCount;
        params->padding0 = 0;
        params->padding1 = 0;

        RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

        // XY 축은 한 스레드 그룹이 몇개의 타일을 계산할 것인지를 기준으로 스레드 그룹 개수를 계산.
        // Z 축은 한 스레드 그룹이 하나의 mask word 를 처리하는 것을 기준으로 스레드 그룹 개수를 계산.
        const uint32_t threadGroupCountX = (visCamera.lightTileCountX + ShaderLightCullingThreadGroupSizeX - 1) / ShaderLightCullingThreadGroupSizeX;
        const uint32_t threadGroupCountY = (visCamera.lightTileCountY + ShaderLightCullingThreadGroupSizeY - 1) / ShaderLightCullingThreadGroupSizeY;
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, visCamera.lightMaskWordCount);
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lightRefBuffer.buffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(lightTileMaskBuffer.buffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    lightRefBuffer.state = RHI::GPUResourceState::ShaderResource;
    lightTileMaskBuffer.state = RHI::GPUResourceState::ShaderResource;
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

static BE_INLINE uint32_t PackLightTileCoord(uint32_t x, uint32_t y) {
    assert(x <= ShaderLightTileCoordMask);
    assert(y <= ShaderLightTileCoordMask);
    return x | (y << ShaderLightTileCoordBitCount);
}

void ShaderLightCullingTable::Build(const VisCamera &visCamera, const VisLight &visLight, ShaderLightRef &shaderLightRef) const {
    const uint32_t lightType = static_cast<uint32_t>(visLight.type);
    assert(lightType <= ShaderLightTypeMask);

    // 하위 비트: 라이트 타입, 상위 비트: 라이트 타입 별 인덱스
    shaderLightRef.packedLightRef = (visLight.typeIndex << ShaderLightTypeBitCount) | lightType;

    if (visCamera.outputRect.w <= 0 || visCamera.outputRect.h <= 0 ||
        visCamera.renderWidth == 0 || visCamera.renderHeight == 0) {
        assert(0);
        shaderLightRef.packedTileMin = 0;
        shaderLightRef.packedTileMax = 0;
        return;
    }

    const int64_t outputMinX = visCamera.outputRect.x;
    const int64_t outputMinY = visCamera.outputRect.y;
    const int64_t outputMaxX = outputMinX + visCamera.outputRect.w;
    const int64_t outputMaxY = outputMinY + visCamera.outputRect.h;

    int64_t clipMinX = visLight.screenClipRect.x;
    int64_t clipMinY = visLight.screenClipRect.y;
    int64_t clipMaxX = static_cast<int64_t>(visLight.screenClipRect.x) + visLight.screenClipRect.w;
    int64_t clipMaxY = static_cast<int64_t>(visLight.screenClipRect.y) + visLight.screenClipRect.h;

    // 1 픽셀보다 작은 투영 영역은 정수 Rect 에서 크기가 0 이 될 수 있으므로 최소 한 픽셀을 보수적으로 포함한다.
    if (clipMinX >= clipMaxX && visLight.screenClipRect.w == 0) {
        clipMinX = std::min(std::max<int64_t>(visLight.screenClipRect.x, outputMinX), outputMaxX - 1);
        clipMaxX = clipMinX + 1;
    }
    if (clipMinY >= clipMaxY && visLight.screenClipRect.h == 0) {
        clipMinY = std::min(std::max<int64_t>(visLight.screenClipRect.y, outputMinY), outputMaxY - 1);
        clipMaxY = clipMinY + 1;
    }

    if (clipMinX >= clipMaxX || clipMinY >= clipMaxY) {
        assert(0);
        shaderLightRef.packedTileMin = 0;
        shaderLightRef.packedTileMax = 0;
        return;
    }

    const uint64_t outputWidth = static_cast<uint64_t>(visCamera.outputRect.w);
    const uint64_t outputHeight = static_cast<uint64_t>(visCamera.outputRect.h);
    const uint64_t renderWidth = visCamera.renderWidth;
    const uint64_t renderHeight = visCamera.renderHeight;

    // screenClipRect 는 outputRect 좌표계이므로, 동적 해상도를 반영한 렌더 타겟 좌표계로 변환한다.
    const uint64_t renderMinX = static_cast<uint64_t>(clipMinX - outputMinX) * renderWidth / outputWidth;
    const uint64_t renderMinY = static_cast<uint64_t>(clipMinY - outputMinY) * renderHeight / outputHeight;
    const uint64_t renderMaxX = (static_cast<uint64_t>(clipMaxX - outputMinX) * renderWidth + outputWidth - 1) / outputWidth;
    const uint64_t renderMaxY = (static_cast<uint64_t>(clipMaxY - outputMinY) * renderHeight + outputHeight - 1) / outputHeight;

    // 렌더 타겟 좌표계 기준으로 타일 좌표 (MinX, MinY, MaxX, MaxY) 를 구한다.
    const uint32_t tileMinX = static_cast<uint32_t>(renderMinX / ShaderLightTileSize);
    const uint32_t tileMinY = static_cast<uint32_t>(renderMinY / ShaderLightTileSize);
    const uint32_t tileMaxX = static_cast<uint32_t>((renderMaxX - 1) / ShaderLightTileSize);
    const uint32_t tileMaxY = static_cast<uint32_t>((renderMaxY - 1) / ShaderLightTileSize);

    shaderLightRef.packedTileMin = PackLightTileCoord(tileMinX, tileMinY);
    shaderLightRef.packedTileMax = PackLightTileCoord(tileMaxX, tileMaxY);
}
