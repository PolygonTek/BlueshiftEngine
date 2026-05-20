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
#include "Texture.h"
#include "LoadingSystem.h"
#include "RenderSystem.h"

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
    UpdateStreamingTextures();

    CleanExpiredTextures();
}

std::shared_ptr<Texture> TextureManager::AllocTexture(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("TextureManager::AllocTexture: empty texture name");
    }
    if (const auto *entry = textureHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("TextureManager::AllocTexture: %s texture already allocated", hashName);
        }
        textureHashMap.Remove(hashName);
    }

    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->hashName = hashName;
    texture->name = hashName;
    texture->name.StripPath();
    texture->name.StripFileExtension();
    texture->index = nextTextureIndex++;
    textureHashMap.Set(texture->hashName, texture);

    return texture;
}

std::shared_ptr<Texture> TextureManager::FindTexture(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = textureHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

std::shared_ptr<Texture> TextureManager::GetTexture(const char *hashName, Texture::Flag flags) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    std::shared_ptr<Texture> texture = FindTexture(hashName);
    if (texture) {
        return texture;
    }

    texture = AllocTexture(hashName);
#ifdef USE_ASYNC_LOADING
    if (!texture->LoadAsync(hashName, flags)) {
#else
    if (!texture->Load(hashName, flags)) {
#endif
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
    if (whiteTexture) {
        whiteTexture.reset();
    }
    if (flatNormalTexture) {
        flatNormalTexture.reset();
    }
}

void TextureManager::RegisterStreamingTexture(const std::shared_ptr<Texture> &texture, const BE1::Image::MipRange &mipRange) {
    if (!texture) {
        return;
    }

    Texture::Streaming &streaming = texture->streaming;
    if (streaming.streamingListIndex >= 0) {
        BE_WARNLOG("TextureManager::RegisterStreamingTexture: texture '%s' has already registered\n", texture->hashName.c_str());
        return;
    }

    int index = streamingTextures.Append(std::weak_ptr<Texture>(texture));

    streaming.streamingListIndex = index;
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
    int index = streaming.streamingListIndex;
    if (index >= 0 && index < streamingTextures.Count()) {
        RemoveStreamingTextureAt(index);
    }

    streaming = {};
}

void TextureManager::RemoveStreamingTextureAt(int index) {
    int lastIndex = streamingTextures.Count() - 1;
    if (index != lastIndex) {
        streamingTextures[index] = streamingTextures[lastIndex];

        if (auto movedTexture = streamingTextures[index].lock()) {
            movedTexture->streaming.streamingListIndex = index;
        }
    }

    streamingTextures.RemoveIndexFast(lastIndex);
}

void TextureManager::UpdateStreamingTextures() {
    for (int streamingTextureIndex = 0; streamingTextureIndex < streamingTextures.Count(); ++streamingTextureIndex) {
        std::shared_ptr<Texture> texture = streamingTextures[streamingTextureIndex].lock();
        if (!texture) {
            RemoveStreamingTextureAt(streamingTextureIndex);
            --streamingTextureIndex;
            continue;
        }

        Texture::Streaming &streaming = texture->streaming;

        // 교체할 텍스쳐가 존재한다면..
        if (streaming.pendingReplacementTexture) {
            RHI::GPUResourceStatus status = streaming.pendingReplacementTexture->GetStatus();

            if (status == RHI::GPUResourceStatus::Ready || status == RHI::GPUResourceStatus::Failed) {
                if (status == RHI::GPUResourceStatus::Ready) {
                    // 기존 텍스쳐를 파괴하고, 새로 로딩된 텍스쳐로 교체한다.
                    RHI::renderer->DestroyTexture(texture->rhiTexture);
                    texture->rhiTexture = streaming.pendingReplacementTexture;

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

        // 요청된 텍스쳐 해상도를 계산한다. (최소 크기는 TextureManager::MinResidentStreamingTextureMaxDimension 로 제한한다)
        uint32_t requestedMaxDimension = BE1::Max(streaming.requestedMaxDimension, TextureManager::MinResidentStreamingTextureMaxDimension);
        streaming.requestedMaxDimension = 0;

        // 요청되었던 최대 크기로 원하는 first mip 레벨을 구한다.
        int desiredFirstMip = BE1::Image::ComputeFirstMipForMaxDimension(streaming.originalWidth, streaming.originalHeight, streaming.originalDepth, streaming.originalMipLevels, requestedMaxDimension);
        if (desiredFirstMip <= streaming.residentFirstMip) {
            // 같거나 더 높은 해상도가 필요한 경우, 딜레이를 초기화한다.
            streaming.unloadDelay = 0;

            if (desiredFirstMip < streaming.residentFirstMip) {
                // 더 높은 해상도가 필요한 경우, 부드러운 스트리밍을 원한다면 한번에 한 단계만 낮춘다.
                desiredFirstMip = BE1::Max(desiredFirstMip, streaming.residentFirstMip - 1);
            } else {
                continue;
            }
        } else {
            // 더 낮은 해상도가 필요한 경우, 바로 낮추지 않고 일정 프레임 딜레이를 준다.
            streaming.unloadDelay++;

            // 비디오 메모리 사용량이 75% 이상이라면 더 빠르게 낮추도록 한다.
            float videoMemoryUsageRatio = (float)(renderSystem->GetVideoMemoryUsage().usageBytes) / (float)(renderSystem->GetVideoMemoryUsage().budgetBytes);
            uint32_t targetUnloadDelay = videoMemoryUsageRatio >= 0.75f ? 5 : 300;

            if (streaming.unloadDelay < targetUnloadDelay) {
                continue;
            }
        }

        // 현재 요청하는 first mip 레벨을 기록한다.
        streaming.requestedFirstMip = desiredFirstMip;

        if (!loadingSystem.EnqueueTextureLoadAndReplace(texture, texture->hashName, streaming.requestedFirstMip)) {
            streaming.requestedFirstMip = -1;
        }
    }
}
