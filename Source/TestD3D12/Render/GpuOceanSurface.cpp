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
#include "GpuOcean.h"
#include "Render/Texture/Texture.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"
#include "Math/AABB.h"
#include "Math/OBB.h"
#include "Math/Frustum.h"

static constexpr uint32_t OceanSurfaceGridLodSteps[] = { 1u, 3u, 5u };

static_assert(GpuOcean::CascadeCount == OceanCascadeCount);
static_assert(COUNT_OF(OceanSurfaceGridLodSteps) == GpuOcean::SurfaceGridLodCount);
static_assert(GpuOcean::SurfaceGridLodCount == OceanSurfaceGridLodCount);
static_assert(GpuOcean::SurfaceGridLodCount == 3u);
static_assert((GpuOcean::SurfaceGridResolution - 1u) % OceanSurfaceGridLodSteps[GpuOcean::SurfaceGridLodCount - 1u] == 0u);

static constexpr uint32_t GetOceanSurfaceGridLodIndexCount(uint32_t lodLevel) {
    const uint32_t quadCountPerAxis = (GpuOcean::SurfaceGridResolution - 1u) / OceanSurfaceGridLodSteps[lodLevel];
    return quadCountPerAxis * quadCountPerAxis * 6u;
}

bool GpuOcean::CreateSurfaceGrid() {
    static_assert(CascadeCount == OceanCascadeCount);
    static_assert(SurfaceGridResolution == OceanSurfaceGridResolution);
    static_assert(SurfaceGridResolution * SurfaceGridResolution <= 1u << 16u);

    if (surfaceGridIndexBuffer) {
        return true;
    }

    DestroySurfaceGrid();

    uint32_t totalIndexCount = 0;
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        surfaceGridLodRanges[lodLevel].firstIndex = totalIndexCount;
        surfaceGridLodRanges[lodLevel].indexCount = GetOceanSurfaceGridLodIndexCount(lodLevel);
        totalIndexCount += surfaceGridLodRanges[lodLevel].indexCount;
    }

    BE1::Array<uint16_t> indices;
    indices.SetCount(totalIndexCount);

    uint32_t indexOffset = 0;
    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        const uint32_t gridStep = OceanSurfaceGridLodSteps[lodLevel];
        const SurfaceGridLodRange &lodRange = surfaceGridLodRanges[lodLevel];
        assert(indexOffset == lodRange.firstIndex);

        for (uint32_t y = 0; y < SurfaceGridResolution - 1; y += gridStep) {
            for (uint32_t x = 0; x < SurfaceGridResolution - 1; x += gridStep) {
                const uint32_t bottomLeft = y * SurfaceGridResolution + x;
                const uint32_t bottomRight = bottomLeft + gridStep;
                const uint32_t topLeft = bottomLeft + gridStep * SurfaceGridResolution;
                const uint32_t topRight = topLeft + gridStep;

                indices[indexOffset++] = static_cast<uint16_t>(bottomLeft);
                indices[indexOffset++] = static_cast<uint16_t>(bottomRight);
                indices[indexOffset++] = static_cast<uint16_t>(topLeft);
                indices[indexOffset++] = static_cast<uint16_t>(topLeft);
                indices[indexOffset++] = static_cast<uint16_t>(bottomRight);
                indices[indexOffset++] = static_cast<uint16_t>(topRight);
            }
        }
        assert(indexOffset == lodRange.firstIndex + lodRange.indexCount);
    }

    surfaceGridIndexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indices[0]), totalIndexCount, indices.Ptr());
    if (!surfaceGridIndexBuffer) {
        BE_WARNLOG("GpuOcean::CreateSurfaceGrid: failed to create packed surface grid index buffer\n");
        DestroySurfaceGrid();
        return false;
    }
    RHI::renderer->SetName(surfaceGridIndexBuffer, "GpuOcean::surfaceGridIndexBuffer");
    return true;
}

void GpuOcean::DestroySurfaceGrid() {
    if (surfaceGridIndexBuffer) {
        RHI::renderer->DestroyIndexBuffer(surfaceGridIndexBuffer);
        surfaceGridIndexBuffer = nullptr;
    }
    for (SurfaceGridLodRange &lodRange : surfaceGridLodRanges) {
        lodRange = {};
    }
}

static uint32_t GetOceanSurfaceTileLodLevel(float tileX, float tileY) {
    const float tileRingDistance = BE1::Max(BE1::Math::Abs(tileX), BE1::Math::Abs(tileY));
    if (tileRingDistance <= 1.5f) {
        return 0u;
    }
    if (tileRingDistance <= 2.5f) {
        return 1u;
    }
    return 2u;
}

static constexpr uint32_t MaxOceanSurfaceTileCount = GpuOcean::MaxSurfaceGridTileCountPerAxis * GpuOcean::MaxSurfaceGridTileCountPerAxis;

struct OceanSurfaceTileBatch {
    OceanSurfaceTileInstance        instances[MaxOceanSurfaceTileCount];
    uint32_t                        lodInstanceCounts[GpuOcean::SurfaceGridLodCount];
    uint32_t                        lodInstanceOffsets[GpuOcean::SurfaceGridLodCount];
    uint32_t                        instanceCount;
    float                           gridSnapSpacing;
};

static bool BuildOceanSurfaceTileBatch(const GpuOcean::SurfaceSettings &settings, const GpuOcean::SurfaceView &view, OceanSurfaceTileBatch &batch) {
    batch = {};

    const bool hasPerspectiveFrustum = view.frustum != nullptr;
    const bool hasOrthographicBox = view.box != nullptr;
    if (settings.frustumCulling && hasPerspectiveFrustum == hasOrthographicBox) {
        return false;
    }

    const uint32_t tileCountPerAxis = settings.gridTileCountPerAxis;
    const float tileCenterOffset = static_cast<float>(tileCountPerAxis - 1u) * 0.5f;

    uint32_t tileLodLevels[MaxOceanSurfaceTileCount] = {};
    uint32_t tileLodCounts[GpuOcean::SurfaceGridLodCount] = {};
    for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
        for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
            const float tilePositionX = static_cast<float>(tileX) - tileCenterOffset;
            const float tilePositionY = static_cast<float>(tileY) - tileCenterOffset;
            const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
            const uint32_t lodLevel = GetOceanSurfaceTileLodLevel(tilePositionX, tilePositionY);
            tileLodLevels[tileIndex] = lodLevel;
            tileLodCounts[lodLevel]++;
        }
    }

    // 모든 tile LOD 의 world-space 정점 격자 위상이 유지되도록 활성 stride 의 최소공배수 간격으로 grid 원점을 이동한다.
    const uint32_t gridSnapStep = tileLodCounts[2] > 0u ? 15u : (tileLodCounts[1] > 0u ? 3u : 1u);
    batch.gridSnapSpacing = settings.gridSpacing * static_cast<float>(gridSnapStep);
    const BE1::Vec2 snappedCameraPosition(
        BE1::Math::Round(view.origin.x / batch.gridSnapSpacing) * batch.gridSnapSpacing,
        BE1::Math::Round(view.origin.y / batch.gridSnapSpacing) * batch.gridSnapSpacing);

    const float tileSize = static_cast<float>(GpuOcean::SurfaceGridResolution - 1u) * settings.gridSpacing;
    const float tileHalfSize = tileSize * 0.5f;
    BE1::Frustum expandedSurfaceFrustum;
    const BE1::Frustum *surfaceFrustum = view.frustum;
    if (settings.frustumCulling && hasPerspectiveFrustum && view.infiniteZFar) {
        expandedSurfaceFrustum = *view.frustum;

        // Infinite-far 투영에서 유한 camera far plane 때문에 실제로 투영 가능한 외곽 tile 이 제거되지 않도록 전체 grid 를 포함한다.
        const float gridHalfSize = static_cast<float>(tileCountPerAxis) * tileHalfSize;
        const float maxOffsetX = BE1::Math::Abs(snappedCameraPosition.x - view.origin.x) + gridHalfSize + settings.maxDisplacement;
        const float maxOffsetY = BE1::Math::Abs(snappedCameraPosition.y - view.origin.y) + gridHalfSize + settings.maxDisplacement;
        const float maxOffsetZ = BE1::Math::Abs(settings.height - view.origin.z) + settings.maxDisplacement;
        const float requiredFarDistance = BE1::Math::Sqrt(maxOffsetX * maxOffsetX + maxOffsetY * maxOffsetY + maxOffsetZ * maxOffsetZ);
        if (requiredFarDistance > expandedSurfaceFrustum.GetFarDistance()) {
            expandedSurfaceFrustum.MoveFarDistance(requiredFarDistance);
        }
        surfaceFrustum = &expandedSurfaceFrustum;
    }

    bool tileVisible[MaxOceanSurfaceTileCount] = {};
    if (settings.frustumCulling) {
        for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
            for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
                const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
                const float tilePositionX = static_cast<float>(tileX) - tileCenterOffset;
                const float tilePositionY = static_cast<float>(tileY) - tileCenterOffset;
                const BE1::Vec3 tileCenter(
                    snappedCameraPosition.x + tilePositionX * tileSize,
                    snappedCameraPosition.y + tilePositionY * tileSize,
                    settings.height);

                // FFT field 의 최대 변위를 CPU 에서 알 수 없으므로 설정된 보수적 최대값으로 모든 축을 확장한다.
                const BE1::AABB tileBounds(
                    tileCenter - BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f),
                    tileCenter + BE1::Vec3(tileHalfSize, tileHalfSize, 0.0f));
                const BE1::AABB expandedTileBounds = tileBounds.Expand(settings.maxDisplacement);
                const bool culled = hasPerspectiveFrustum
                    ? surfaceFrustum->CullAABB(expandedTileBounds)
                    : !view.box->IsIntersectAABB(expandedTileBounds);
                if (!culled) {
                    tileVisible[tileIndex] = true;
                    batch.instanceCount++;
                    batch.lodInstanceCounts[tileLodLevels[tileIndex]]++;
                }
            }
        }
    } else {
        batch.instanceCount = tileCountPerAxis * tileCountPerAxis;
        for (uint32_t tileIndex = 0; tileIndex < batch.instanceCount; ++tileIndex) {
            tileVisible[tileIndex] = true;
        }
        for (uint32_t lodLevel = 0; lodLevel < GpuOcean::SurfaceGridLodCount; ++lodLevel) {
            batch.lodInstanceCounts[lodLevel] = tileLodCounts[lodLevel];
        }
    }

    if (batch.instanceCount == 0u) {
        return true;
    }

    uint32_t lodInstanceWriteOffsets[GpuOcean::SurfaceGridLodCount] = {};
    for (uint32_t lodLevel = 1; lodLevel < GpuOcean::SurfaceGridLodCount; ++lodLevel) {
        batch.lodInstanceOffsets[lodLevel] = batch.lodInstanceOffsets[lodLevel - 1u] + batch.lodInstanceCounts[lodLevel - 1u];
    }
    for (uint32_t lodLevel = 0; lodLevel < GpuOcean::SurfaceGridLodCount; ++lodLevel) {
        lodInstanceWriteOffsets[lodLevel] = batch.lodInstanceOffsets[lodLevel];
    }

    for (uint32_t tileY = 0; tileY < tileCountPerAxis; ++tileY) {
        for (uint32_t tileX = 0; tileX < tileCountPerAxis; ++tileX) {
            const uint32_t tileIndex = tileY * tileCountPerAxis + tileX;
            if (!tileVisible[tileIndex]) {
                continue;
            }
            const uint32_t lodLevel = tileLodLevels[tileIndex];
            auto GetNeighborLodLevel = [&](int32_t neighborX, int32_t neighborY) {
                if (neighborX < 0 || neighborY < 0 || neighborX >= static_cast<int32_t>(tileCountPerAxis) || neighborY >= static_cast<int32_t>(tileCountPerAxis)) {
                    return lodLevel;
                }
                return tileLodLevels[neighborY * tileCountPerAxis + neighborX];
            };

            const uint32_t leftLodLevel = GetNeighborLodLevel(static_cast<int32_t>(tileX) - 1, tileY);
            const uint32_t rightLodLevel = GetNeighborLodLevel(static_cast<int32_t>(tileX) + 1, tileY);
            const uint32_t bottomLodLevel = GetNeighborLodLevel(tileX, static_cast<int32_t>(tileY) - 1);
            const uint32_t topLodLevel = GetNeighborLodLevel(tileX, static_cast<int32_t>(tileY) + 1);

            OceanSurfaceTileInstance &tileInstance = batch.instances[lodInstanceWriteOffsets[lodLevel]++];
            tileInstance.tilePosition = float2(static_cast<float>(tileX) - tileCenterOffset, static_cast<float>(tileY) - tileCenterOffset);
            tileInstance.lodLevel = lodLevel;
            // left, right, bottom, top 순서로 이웃 LOD 를 8비트씩 패킹한다.
            tileInstance.packedNeighborLodLevels = leftLodLevel | (rightLodLevel << 8u) | (bottomLodLevel << 16u) | (topLodLevel << 24u);
        }
    }
    for (uint32_t lodLevel = 0; lodLevel < GpuOcean::SurfaceGridLodCount; ++lodLevel) {
        assert(lodInstanceWriteOffsets[lodLevel] == batch.lodInstanceOffsets[lodLevel] + batch.lodInstanceCounts[lodLevel]);
    }

    return true;
}

bool GpuOcean::DrawSurface(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
    const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const SurfaceSettings &surfaceSettings, const SurfaceView &surfaceView, SurfaceDrawStats *drawStats) const {
    PROFILER_CPU_SCOPED_EVENT("GpuOcean::DrawSurface");

    assert(surfaceSettings.IsValid());

    if (drawStats) {
        *drawStats = {};
    }

    if (!pipelineState || !frameConstantBuffer || !cameraConstantBuffer || !surfaceGridIndexBuffer || !cascadeOutputsReady) {
        return false;
    }

    if (!fieldTextureArray || fieldTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !foamHistoryTextureArray || foamHistoryTextureArray->GetStatus() != RHI::GPUResourceStatus::Ready) {
        return false;
    }

    RHI::Texture *foamDetailMap = foamDetailTexture ? foamDetailTexture->GetReadyRHITexture() : nullptr;
    if (!foamDetailMap && textureManager.whiteTexture) {
        foamDetailMap = textureManager.whiteTexture->GetReadyRHITexture();
    }
    if (!foamDetailMap) {
        return false;
    }

    RHI::Texture *waterDetailNormalMap = waterDetailNormalTexture ? waterDetailNormalTexture->GetReadyRHITexture() : nullptr;
    if (!waterDetailNormalMap && textureManager.flatNormalTexture) {
        waterDetailNormalMap = textureManager.flatNormalTexture->GetReadyRHITexture();
    }
    if (!waterDetailNormalMap) {
        return false;
    }

    OceanSurfaceTileBatch tileBatch;
    if (!BuildOceanSurfaceTileBatch(surfaceSettings, surfaceView, tileBatch)) {
        return false;
    }
    if (tileBatch.instanceCount == 0u) {
        return true;
    }

    RHI::Buffer *tileInstanceBuffer = frameThreadData->AllocStructuredBuffer(sizeof(OceanSurfaceTileInstance), tileBatch.instanceCount, tileBatch.instances);
    if (!tileInstanceBuffer) {
        return false;
    }

    // 캐스케이드 별 패치 사이즈 (L) 와 grid spacing 에 맞는 필드 텍스쳐의 mip LOD 계산
    float cascadePatchSizes[4] = {};
    float cascadeVertexMipLevels[4] = {};
    const float fieldTextureResolution = static_cast<float>(fieldTextureArray->GetWidth());
    const float maxFieldMipLevel = static_cast<float>(fieldTextureArray->GetMipLevelCount() - 1u);
    for (uint32_t cascadeIndex = 0; cascadeIndex < CascadeCount; ++cascadeIndex) {
        const float patchSize = simulationSettings.cascades[cascadeIndex].patchSize;
        cascadePatchSizes[cascadeIndex] = patchSize;
        cascadeVertexMipLevels[cascadeIndex] = BE1::Clamp(BE1::Math::Log2(surfaceSettings.gridSpacing * fieldTextureResolution / patchSize), 0.0f, maxFieldMipLevel);
    }

    OceanSurfaceParams params = {};
    params.cascadePatchSizes = float4(cascadePatchSizes);
    params.cascadeVertexMipLevels = float4(cascadeVertexMipLevels);
    params.gridSpacing = surfaceSettings.gridSpacing;
    params.surfaceHeight = surfaceSettings.height;
    params.roughness = surfaceSettings.roughness;
    params.normalMapSize = surfaceSettings.normalMapSize;
    params.normalStrength = surfaceSettings.normalStrength;
    BE1::Vec2 windFlowDirection = -simulationSettings.windDirection;
    windFlowDirection.Normalize();
    const float normalMapScrollSpeed = simulationSettings.windSpeed * surfaceSettings.normalScrollSpeedScale;
    const float normalMapUVOffsetScale = simulationTime * normalMapScrollSpeed / surfaceSettings.normalMapSize;
    params.normalMapUVOffset = float2(windFlowDirection.x * normalMapUVOffsetScale, windFlowDirection.y * normalMapUVOffsetScale);
    const BE1::Color3 linearWaterColor = surfaceSettings.waterColor.SRGBToLinear();
    params.waterColor = float3(linearWaterColor.r, linearWaterColor.g, linearWaterColor.b);
    params.fresnelF0 = surfaceSettings.fresnelF0;
    const BE1::Color3 linearFoamColor = surfaceSettings.foamColor.SRGBToLinear();
    params.foamColor = float3(linearFoamColor.r, linearFoamColor.g, linearFoamColor.b);
    params.foamDetailMapSize = surfaceSettings.foamDetailMapSize;
    params.foamDetailStrength = surfaceSettings.foamDetailStrength;
    params.displacementScale = simulationSettings.displacementScale;
    params.cascadeFadeScale = surfaceSettings.cascadeFadeScale;
    params.gridSnapSpacing = tileBatch.gridSnapSpacing;
    params.foamOptions = simulationSettings.foamEnabled ? OceanFoamOptionBitEnabled : 0u;

    RHI::ConstantBuffer *surfaceConstantBuffer = frameThreadData->AllocConstant(sizeof(params), &params);
    if (!surfaceConstantBuffer) {
        return false;
    }

    RHI::renderer->SetPSO(commandList, pipelineState);
    RHI::renderer->SetIndexBuffer(commandList, surfaceGridIndexBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 1, cameraConstantBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 2, surfaceConstantBuffer);

    RHI::renderer->SetTexture(commandList, 0, false, fieldTextureArray);
    RHI::renderer->SetTexture(commandList, 1, false, foamHistoryTextureArray);
    RHI::renderer->SetTexture(commandList, 2, false, foamDetailMap);
    RHI::renderer->SetTexture(commandList, 3, false, waterDetailNormalMap);
    RHI::renderer->SetBuffer(commandList, 4, false, tileInstanceBuffer);

    if (drawStats) {
        drawStats->instanceBufferAllocCount = 1;
        drawStats->instanceBufferUploadBytes = sizeof(OceanSurfaceTileInstance) * tileBatch.instanceCount;
    }

    for (uint32_t lodLevel = 0; lodLevel < SurfaceGridLodCount; ++lodLevel) {
        const uint32_t instanceCount = tileBatch.lodInstanceCounts[lodLevel];
        if (instanceCount == 0) {
            continue;
        }

        const SurfaceGridLodRange &lodRange = surfaceGridLodRanges[lodLevel];
        const uint32_t tileInstanceOffset = tileBatch.lodInstanceOffsets[lodLevel];
        RHI::renderer->SetConstants(commandList, 3, &tileInstanceOffset, sizeof(tileInstanceOffset), 0);
        RHI::renderer->DrawIndexedInstanced(commandList, lodRange.indexCount, instanceCount, lodRange.firstIndex, 0, 0);

        if (drawStats) {
            drawStats->drawCount++;
            drawStats->triangleCount += static_cast<uint64_t>(lodRange.indexCount / 3u) * instanceCount;
        }
    }
    return true;
}
