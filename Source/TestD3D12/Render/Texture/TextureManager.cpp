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
#include "Render/RenderCvars.h"
#include "TextureManager.h"
#include "LoadingSystem.h"

TextureManager      textureManager;

void TextureManager::Init() {
    textureHashMap.Init(1024, 1024, 1024);

    expiredTextureHashMapCursor = 0;

    nextTextureIndex = 0;

    CreateEngineTextures();
}

void TextureManager::Shutdown() {
    DestroyEngineTextures();

    textureHashMap.Clear();

    streamingTextures.Clear();
}

void TextureManager::UpdateFrame() {
#ifdef USE_MIPMAP_STREAMING
    UpdateStreamingTextures();
#endif

    CleanExpiredTextures();
}

SharedPtr<Texture> TextureManager::AllocTexture(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("TextureManager::AllocTexture: empty texture name");
    }
    if (const auto *entry = textureHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("TextureManager::AllocTexture: %s texture already allocated", hashName);
        }
        textureHashMap.Remove(hashName);
    }

    SharedPtr<Texture> texture = MakeShared<Texture>();
    texture->hashName = hashName;
    texture->name = hashName;
    texture->name.StripPath();
    texture->name.StripFileExtension();
    texture->index = nextTextureIndex++;
    textureHashMap.Set(texture->hashName, texture);

    return texture;
}

SharedPtr<Texture> TextureManager::FindTexture(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = textureHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

SharedPtr<Texture> TextureManager::GetTexture(const char *hashName, Texture::Flag flags, bool async) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    SharedPtr<Texture> texture = FindTexture(hashName);
    if (texture) {
        if (!async && !texture->WaitReady()) {
            return defaultTexture;
        }
        return texture;
    }

    texture = AllocTexture(hashName);
#ifdef USE_ASYNC_LOADING
    bool loaded = async ? texture->LoadAsync(hashName, flags) : texture->Load(hashName, flags);
#else
    bool loaded = texture->Load(hashName, flags);
#endif
    if (!loaded) {
        textureHashMap.Remove(texture->hashName);
        return defaultTexture;
    }

    return texture;
}

void TextureManager::CleanExpiredTextures() {
    constexpr int ExpiredTextureHashMapScanBudget = 32;
    int scansLeft = ExpiredTextureHashMapScanBudget;

    while (scansLeft-- > 0 && textureHashMap.Count() > 0) {
        if (expiredTextureHashMapCursor >= textureHashMap.Count()) {
            expiredTextureHashMapCursor = 0;
        }

        const auto *entry = textureHashMap.GetByIndex(expiredTextureHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            textureHashMap.Remove(entry->first);
            continue;
        }

        expiredTextureHashMapCursor++;
    }
}

void TextureManager::CreateEngineTextures() {
    // Create default texture.
    defaultTexture = AllocTexture("_defaultTexture");
    defaultTexture->CreateDefaultTexture(16);

    // Create black texture.
    blackTexture = AllocTexture("_blackTexture");
    blackTexture->CreateColorTexture(8, BE1::Color4::black);

    // Create white texture.
    whiteTexture = AllocTexture("_whiteTexture");
    whiteTexture->CreateColorTexture(8, BE1::Color4::white);

    // Create flatNormal texture.
    flatNormalTexture = AllocTexture("_flatNormalTexture");
    flatNormalTexture->CreateFlatNormalTexture(16);
}

void TextureManager::DestroyEngineTextures() {
    if (defaultTexture) {
        defaultTexture.reset();
    }
    if (blackTexture) {
        blackTexture.reset();
    }
    if (whiteTexture) {
        whiteTexture.reset();
    }
    if (flatNormalTexture) {
        flatNormalTexture.reset();
    }
}

bool TextureManager::IsRegisteredStreamingTexture(const Texture *texture) const {
    if (!texture) {
        return false;
    }

    return texture->streaming.streamingListIndex != TextureStreamingInvalidIndex;
}

void TextureManager::RegisterStreamingTexture(const SharedPtr<Texture> &texture, const BE1::Image::MipRange &mipRange) {
    if (!texture) {
        return;
    }

    Texture::Streaming &streaming = texture->streaming;

    if (streaming.streamingListIndex != TextureStreamingInvalidIndex) {
        BE_WARNLOG("TextureManager::RegisterStreamingTexture: texture '%s' has already registered\n", texture->hashName.c_str());
        return;
    }

    streaming.streamingListIndex = streamingTextures.Append(WeakPtr<Texture>(texture));
    streaming.originalWidth = mipRange.originalWidth;
    streaming.originalHeight = mipRange.originalHeight;
    streaming.originalDepth = mipRange.originalDepth;
    streaming.originalMipLevels = mipRange.originalMipLevels;
    streaming.residentFirstMip = mipRange.firstMip;
    streaming.requestedFirstMip = -1;
    streaming.requestedMaxDimension = 0;
    streaming.pendingReplacementTexture = nullptr;
}

void TextureManager::UnregisterStreamingTexture(Texture *texture) {
    if (!texture) {
        return;
    }

    Texture::Streaming &streaming = texture->streaming;

    // 교체 예정이었던 텍스쳐 파괴
    if (streaming.pendingReplacementTexture) {
        RHI::renderer->DestroyTexture(streaming.pendingReplacementTexture);
        streaming.pendingReplacementTexture = nullptr;
    }

    // 스트리밍 텍스쳐 목록에서 제거
    if (streamingTextures.IsValidIndex(streaming.streamingListIndex)) {
        streamingTextures[streaming.streamingListIndex].reset();
    }

    streaming = {};
}

uint32_t TextureManager::GetStreamingTextureMinResidentMaxDimension() const {
    return static_cast<uint32_t>(BE1::Max(1, r_textureStreamingMinResidentMaxDimension.GetInteger()));
}

void TextureManager::UpdateStreamingTextures() {
    int requestedStreamingTextureCount = 0;
    const uint32_t streamInDelay = static_cast<uint32_t>(BE1::Max(0, r_textureStreamingStreamInDelay.GetInteger()));
    const int maxStreamingRequests = BE1::Max(0, r_textureStreamingMaxRequestsPerFrame.GetInteger());
    const int maxStreamingLevelReduction = BE1::Max(0, r_textureStreamingMaxLevelReduction.GetInteger());
    const uint32_t minResidentStreamingTextureMaxDimension = GetStreamingTextureMinResidentMaxDimension();

    ComputeStreamingTextureMemoryUsage();

    uint64_t budgetDebtBytes = 0;
    if (streamingTextureMemoryBudgetBytes > 0) {
        // budget 이 부족하다면 얼마나 부족한지 계산한다.
        budgetDebtBytes = streamingTextureMemoryUsageBytes > streamingTextureMemoryBudgetBytes ? streamingTextureMemoryUsageBytes - streamingTextureMemoryBudgetBytes : 0;
    }

    const int numStreamingTextures = streamingTextures.Count();
    for (int streamingTextureIndex = 0; streamingTextureIndex < numStreamingTextures; ++streamingTextureIndex) {
        SharedPtr<Texture> texture = streamingTextures[streamingTextureIndex].lock();
        if (!texture) {
            continue;
        }

        Texture::Streaming &streaming = texture->streaming;

        // 교체할 텍스쳐가 존재한다면..
        if (streaming.pendingReplacementTexture) {
            RHI::GPUResourceStatus status = streaming.pendingReplacementTexture->GetStatus();

            if (status == RHI::GPUResourceStatus::Ready || status == RHI::GPUResourceStatus::Failed) {
                if (status == RHI::GPUResourceStatus::Ready) {
                    // 기존 텍스쳐를 파괴하고, 새로 로딩된 텍스쳐로 교체한다.
                    RHI::renderer->DestroyTexture(texture->internalData.rhiTexture);
                    texture->internalData.rhiTexture = streaming.pendingReplacementTexture;

                    streaming.residentFirstMip = streaming.requestedFirstMip;
                } else if (status == RHI::GPUResourceStatus::Failed) {
                    // 업로드를 실패했다면 교체하지 않고, 새로 로딩된 텍스쳐를 파괴한다. (나중에 재요청될 것이다)
                    RHI::renderer->DestroyTexture(streaming.pendingReplacementTexture);
                }

                streaming.pendingReplacementTexture = nullptr;
                streaming.requestedFirstMip = -1;
            }
        }

        // 아직 교체가 완료되지 않았다면..
        if (streaming.requestedFirstMip >= 0) {
            continue;
        }

        // 한 프레임에 스트리밍 텍스쳐가 너무 많이 요청되는 것을 방지하기 위해, 최대 개수까지만 요청한다.
        if (requestedStreamingTextureCount < maxStreamingRequests) {
            // 요청된 텍스쳐 해상도를 계산한다. (최소 크기는 r_textureStreamingMinResidentMaxDimension 로 제한한다)
            uint32_t requestedMaxDimension = BE1::Max(streaming.requestedMaxDimension, minResidentStreamingTextureMaxDimension);
            streaming.requestedMaxDimension = 0;

            // 요청되었던 최대 크기로 원하는 first mip 레벨을 구한다.
            int desiredFirstMip = BE1::Image::ComputeFirstMipForMaxDimension(streaming.originalWidth, streaming.originalHeight, streaming.originalDepth, streaming.originalMipLevels, requestedMaxDimension);

            // 너무 높은 레벨 (낮은 해상도) 로 스트리밍되는 것을 방지하기 위해, 최대 레벨 감소량을 r_textureStreamingMaxLevelReduction 로 제한한다.
            int maxLevelReduction = BE1::Min(maxStreamingLevelReduction, streaming.originalMipLevels - 1);
            desiredFirstMip = BE1::Min(desiredFirstMip, maxLevelReduction);

            // budget 이 부족하다면, 레벨을 높인다. (해상도를 낮춘다)
            if (budgetDebtBytes > 0) {
                if (streaming.residentFirstMip >= 0 && streaming.residentFirstMip < maxLevelReduction) {
                    desiredFirstMip = BE1::Max(desiredFirstMip, streaming.residentFirstMip + 1);
                }
            }

            if (desiredFirstMip <= streaming.residentFirstMip) {
                // 같거나 더 낮은 레벨 (높은 해상도) 이 필요한 경우, 딜레이를 초기화한다.
                streaming.streamOutFrameCount = 0;

                if (desiredFirstMip < streaming.residentFirstMip) {
                    streaming.streamInFrameCount++;

                    if (streaming.streamInFrameCount < streamInDelay) {
                        continue;
                    }

                    streaming.streamInFrameCount = 0;

                    // 더 낮은 레벨 (높은 해상도) 이 필요한 경우, 부드러운 스트리밍을 위해 한번에 한 단계만 낮춘다.
                    desiredFirstMip = BE1::Max(desiredFirstMip, streaming.residentFirstMip - 1);
                } else {
                    continue;
                }
            } else {
                // 더 높은 레벨 (낮은 해상도) 이 필요한 경우, 바로 낮추지 않고 일정 프레임 딜레이를 준다.
                streaming.streamOutFrameCount++;
                streaming.streamInFrameCount = 0;

                // budget 사용량에 따라 더 빠르게 낮추도록 한다.
                float videoMemoryUsageRatio = streamingTextureMemoryBudgetBytes > 0 ? (float)(streamingTextureMemoryUsageBytes) / (float)(streamingTextureMemoryBudgetBytes) : 0.0f;
                int targetUnloadFrameCount = budgetDebtBytes > 0 ? 0 : (videoMemoryUsageRatio >= 0.75f ? 5 : 300);

                if (streaming.streamOutFrameCount < targetUnloadFrameCount) {
                    continue;
                }
            }

            uint64_t budgetSavedBytes = 0;

            // budget 이 부족하다면, 레벨을 높이기 전에 현재 레벨과 원하는 레벨의 메모리 사용량 차이를 계산하여, 얼마나 budget 을 절약할 수 있는지 계산한다.
            if (budgetDebtBytes > 0) {
                if (desiredFirstMip > streaming.residentFirstMip) {
                    uint64_t residentBytes = texture->MemRequiredStreaming(streaming.residentFirstMip);
                    uint64_t desiredBytes = texture->MemRequiredStreaming(desiredFirstMip);

                    if (residentBytes > desiredBytes) {
                        budgetSavedBytes = residentBytes - desiredBytes;
                    }
                }
            }

            // 현재 요청하는 first mip 레벨을 기록한다.
            streaming.requestedFirstMip = desiredFirstMip;

            if (loadingSystem.EnqueueTextureStreamingReplacement(texture, texture->hashName, streaming.requestedFirstMip)) {
                if (budgetSavedBytes > 0) {
                    budgetDebtBytes = budgetDebtBytes > budgetSavedBytes ? budgetDebtBytes - budgetSavedBytes : 0;
                }
            } else {
                streaming.requestedFirstMip = -1;
            }

            requestedStreamingTextureCount++;
        }
    }
}

void TextureManager::ComputeStreamingTextureMemoryUsage() {
    uint64_t totalBytes = 0;

    const int numStreamingTextures = streamingTextures.Count();
    for (int i = 0; i < numStreamingTextures; ++i) {
        SharedPtr<Texture> texture = streamingTextures[i].lock();
        if (!texture || !texture->GetRHITexture()) {
            continue;
        }

        totalBytes += BE1::Image::MemRequired(
            texture->GetWidth(),
            texture->GetHeight(),
            texture->GetDepth(),
            texture->GetMipLevelCount(),
            texture->GetArraySize(),
            texture->GetFormat());
    }

    streamingTextureMemoryUsageBytes = totalBytes;
}
