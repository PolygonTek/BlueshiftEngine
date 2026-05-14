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

TextureManager      textureManager;

void TextureManager::Init() {
    textureHashMap.Init(1024, 1024, 1024);

    CreateEngineTextures();
}

void TextureManager::Shutdown() {
    textureHashMap.DeleteContents(true);
}

Texture *TextureManager::AllocTexture(const char *hashName) {
    if (textureHashMap.Get(hashName)) {
        BE_FATALERROR("%s texture already allocated", hashName);
    }

    Texture *texture = new Texture;
    texture->hashName = hashName;
    texture->name = hashName;
    texture->name.StripPath();
    texture->name.StripFileExtension();
    texture->index = textureHashMap.Count();
    texture->refCount = 1;

    textureHashMap.Set(texture->hashName, texture);

    return texture;
}

Texture *TextureManager::FindTexture(const char *hashName) const {
    const auto *entry = textureHashMap.Get(BE1::Str(hashName));
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

Texture *TextureManager::GetTexture(const char *hashName, Texture::Flag flags) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    Texture *texture = FindTexture(hashName);
    if (texture) {
        texture->refCount++;
        return texture;
    }

    texture = AllocTexture(hashName);
#ifdef USE_ASYNC_LOADING
    if (!texture->LoadAsync(hashName, flags)) {
#else
    if (!texture->Load(hashName, flags)) {
#endif
        DestroyTexture(texture);
        return defaultTexture;
    }

    return texture;
}

void TextureManager::ReleaseTexture(Texture *texture) {
    if (texture->refCount > 0) {
        if (--texture->refCount > 0) {
            return;
        }
    }

    if (BE1::HasFlag(texture->flags, Texture::Flag::Permanent)) {
        return;
    }

#ifdef USE_ASYNC_LOADING
    if (texture->GetState() == Texture::State::Loading || (texture->streaming.streamingListIndex >= 0 && texture->streaming.pending)) {
        texture->SetState(Texture::State::DestroyPending);
        texture->BumpLoadSerial(); // 진행 중인 completion 무효화

        textureHashMap.Remove(texture->hashName);
        return;
    }
#endif

    DestroyTexture(texture);
}

void TextureManager::DestroyTexture(Texture *texture) {
    if (texture->refCount > 1) {
        BE_WARNLOG("TextureManager::DestroyTexture: texture '%s' has %i reference count\n", texture->hashName.c_str(), texture->refCount);
    }

    textureHashMap.Remove(texture->hashName);

    // Texture::Flag::Streaming 가 켜져있다고 해서 항상 streaming texture 로 등록되진 않는다.
    if (BE1::HasFlag(texture->flags, Texture::Flag::Streaming)) {
        UnregisterStreamingTexture(texture);
    }

    delete texture;
}

void TextureManager::CreateEngineTextures() {
    // Create default texture.
    defaultTexture = AllocTexture("_defaultTexture");
    defaultTexture->CreateDefaultTexture(16, Texture::Flag::Permanent);

    // Create white texture.
    whiteTexture = AllocTexture("_whiteTexture");
    whiteTexture->CreateColorTexture(8, BE1::Color4::white, Texture::Flag::Permanent);

    // Create flatNormal texture.
    flatNormalTexture = AllocTexture("_flatNormalTexture");
    flatNormalTexture->CreateFlatNormalTexture(16, Texture::Flag::Permanent);
}

void TextureManager::RegisterStreamingTexture(Texture *texture, const BE1::Image::MipRange &mipRange) {
    if (streamingTextures.Find(texture)) {
        BE_WARNLOG("TextureManager::RegisterStreamingTexture: texture '%s' has already registered\n", texture->hashName.c_str());
        return;
    }

    int index = streamingTextures.Append(texture);

    texture->streaming.streamingListIndex = index;
    texture->streaming.originalMipLevels = mipRange.originalMipLevels;
    texture->streaming.residentFirstMip = mipRange.firstMip;
    texture->streaming.residentMipCount = mipRange.mipCount;
    texture->streaming.requestedFirstMip = -1;
    texture->streaming.requestedMipCount = 0;
    texture->streaming.pending = false;
}

void TextureManager::UnregisterStreamingTexture(Texture *texture) {
    int index = texture->streaming.streamingListIndex;
    if (index < 0) {
        return;
    }

    int lastIndex = streamingTextures.Count() - 1;
    Texture *lastTexture = streamingTextures[lastIndex];

    streamingTextures[index] = lastTexture;
    lastTexture->streaming.streamingListIndex = index;

    streamingTextures.RemoveIndexFast(lastIndex);

    texture->streaming = {};
}

void TextureManager::UpdateStreamingTextures(int frameMsec) {
    BE1::Array<Texture *> unregisterTextures;

    for (int i = 0; i < streamingTextures.Count(); ++i) {
        Texture *texture = streamingTextures[i];
        Texture::StreamingState &streaming = texture->streaming;

        if (streaming.pending) {
            continue;
        }

        // ProcessResourceRequests (렌더 스레드) 에서 업로드가 실패할 가능성도 있기 때문에 여기서 스트리밍 목록에서 제거한다.
        if (texture->GetRHITexture()->GetStatus() == RHI::GPUResourceStatus::Failed) {
            unregisterTextures.Append(texture);
            continue;
        }

        // 로딩이 완전히 끝난 상태인데 여기에 들어오면 안된다.
        assert(streaming.residentFirstMip > 0);

        int residentMaxDimension = BE1::Max3(
            BE1::Max(1u, texture->GetWidth() >> streaming.residentFirstMip),
            BE1::Max(1u, texture->GetHeight() >> streaming.residentFirstMip),
            BE1::Max(1u, texture->GetDepth() >> streaming.residentFirstMip));

        // 1024 까지는 한번에 요청하고, 그 이상부터는 한 레벨씩 요청한다.
        int targetMaxDimension = residentMaxDimension < 1048 ? 1048 : (residentMaxDimension << 1);
        
        int desiredFirstMip = BE1::Image::ComputeFirstMipForMaxDimension(texture->GetWidth(), texture->GetHeight(), texture->GetDepth(), streaming.originalMipLevels, targetMaxDimension);
        desiredFirstMip = BE1::Min(desiredFirstMip, streaming.residentFirstMip - 1);

        streaming.requestedFirstMip = desiredFirstMip;
        streaming.requestedMipCount = streaming.residentFirstMip - desiredFirstMip;

        if (!loadingSystem.EnqueueTextureMipRangeLoad(texture, texture->hashName, streaming.requestedFirstMip, streaming.requestedMipCount)) {
            unregisterTextures.Append(texture);
            continue;
        }

        streaming.pending = true;
    }

    for (int i = 0; i < unregisterTextures.Count(); ++i) {
        UnregisterStreamingTexture(unregisterTextures[i]);
    }
}

bool TextureManager::CompleteStreamingTextureRequest(Texture *texture, const BE1::Image::MipRange &loadedMipRange, uint32_t *outResidentFirstMip, uint32_t *outResidentMipCount) {
    if (texture->streaming.streamingListIndex < 0) {
        return false;
    }

    Texture::StreamingState &streaming = texture->streaming;
    streaming.pending = false;

    if (loadedMipRange.firstMip < streaming.residentFirstMip) {
        if (loadedMipRange.firstMip + loadedMipRange.mipCount != streaming.residentFirstMip) {
            BE_ERRLOG("TextureManager::CompleteStreamingTextureRequest: non-contiguous mip range for texture '%s' (loaded [%i, %i], resident [%i, %i])\n",
                texture->GetHashName(),
                loadedMipRange.firstMip,
                loadedMipRange.firstMip + loadedMipRange.mipCount - 1,
                streaming.residentFirstMip,
                streaming.residentFirstMip + streaming.residentMipCount - 1);
            return false;
        }

        int oldResidentLastMip = streaming.residentFirstMip + streaming.residentMipCount;
        streaming.residentFirstMip = loadedMipRange.firstMip;
        streaming.residentMipCount = oldResidentLastMip - streaming.residentFirstMip;
    }

    streaming.requestedFirstMip = -1;
    streaming.requestedMipCount = 0;

    *outResidentFirstMip = streaming.residentFirstMip;
    *outResidentMipCount = streaming.residentMipCount;

    // 완전히 로딩이 끝났다면 등록된 스트리밍 텍스쳐 목록에서 제거한다.
    bool fullyResident = streaming.residentFirstMip == 0 && streaming.residentMipCount == streaming.originalMipLevels;
    if (fullyResident) {
        UnregisterStreamingTexture(texture);
    }
    return true;
}
