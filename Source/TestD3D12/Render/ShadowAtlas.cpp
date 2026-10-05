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
#include "ShadowAtlas.h"

bool ShadowAtlas::Ensure(uint32_t atlasSize, uint32_t maxTileResolution, uint32_t minTileResolution) {
    if (shadowTexture && GetSize() == atlasSize) {
        if (GetMaxTileResolution() == maxTileResolution && allocator.GetMinTileResolution() == minTileResolution) {
            return true;
        }

        // 타일의 최대/최소 해상도만 바뀌었다면 texture 는 유지하고 allocator 의 구성과 핸들만 갱신한다.
        if (!allocator.Init(atlasSize, maxTileResolution, minTileResolution)) {
            return false;
        }
        ClearEntries();
        return true;
    }

    SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
    depthImage->Init2DFromMemory(atlasSize, atlasSize, 1, BE1::Image::Format::D32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);

    const RHI::TextureFlag flags = RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless;
    const RHI::ClearValue clearValue = RHI::ClearValue::DepthStencil(0.0f, 0);
    RHI::Texture *newTexture = RHI::renderer->CreateTexture2D(flags, depthImage, RHI::TextureMipInitMode::FromImage, clearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!newTexture) {
        return false;
    }
    RHI::renderer->SetName(newTexture, "ShadowAtlas::shadowTexture");

    const RHI::ViewID newDSV = RHI::renderer->CreateView(newTexture, RHI::ViewType::DSV, RHI::Format::D32_FLOAT, 0, 1, 0, 1);
    const RHI::ViewID newSRV = RHI::renderer->CreateView(newTexture, RHI::ViewType::SRV, RHI::Format::R32_FLOAT, 0, 1, 0, 1);
    if (!newDSV.IsValid() || !newSRV.IsValid() || !allocator.Init(atlasSize, maxTileResolution, minTileResolution)) {
        RHI::renderer->DestroyTexture(newTexture);
        return false;
    }

    ClearEntries();

    if (shadowTexture) {
        RHI::renderer->DestroyTexture(shadowTexture);
    }

    shadowTexture = newTexture;
    shadowDSV = newDSV;
    shadowSRV = newSRV;

    return true;
}

void ShadowAtlas::Destroy() {
    if (shadowTexture) {
        RHI::renderer->DestroyTexture(shadowTexture);
        shadowTexture = nullptr;
    }
    shadowDSV = RHI::ViewID::InvalidID;
    shadowSRV = RHI::ViewID::InvalidID;
    allocator.Shutdown();
    entries.Clear();
    entryHash.Free();
}

void ShadowAtlas::ResetAllocations() {
    allocator.Reset();
    ClearEntries();
}

void ShadowAtlas::ClearEntries() {
    // Array 의 용량은 유지하되, 더 이상 소유하지 않는 결과 저장소의 참조는 해제한다.
    for (Entry &entry : entries) {
        entry.renderState.reset();
    }
    entries.SetCount(0, false);
    entryHash.Clear();
}

int ShadowAtlas::FindEntryIndex(uint64_t lightId) const {
    const int hash = entryHash.GenerateHash(lightId);
    for (int index = entryHash.First(hash); index != -1; index = entryHash.Next(index)) {
        if (entries[index].lightId == lightId) {
            return index;
        }
    }
    return -1;
}

ShadowAtlas::Entry *ShadowAtlas::FindEntry(uint64_t lightId) {
    const int index = FindEntryIndex(lightId);
    return index >= 0 ? &entries[index] : nullptr;
}

const ShadowAtlas::Entry *ShadowAtlas::FindEntry(uint64_t lightId) const {
    const int index = FindEntryIndex(lightId);
    return index >= 0 ? &entries[index] : nullptr;
}

void ShadowAtlas::KeepEntry(uint64_t lightId, uint32_t requestedResolution, uint32_t tileCount) {
    const int index = FindEntryIndex(lightId);
    if (index < 0) {
        return;
    }

    Entry &entry = entries[index];
    // 축소 할당된 타일은 요청이 같으면 유지하고, 실제 해상도가 새 요청과 같아도 유지한다.
    if (entry.numTiles == tileCount && (entry.requestedResolution == requestedResolution || entry.tiles[0].resolution == requestedResolution)) {
        entry.requestedResolution = requestedResolution;
        entry.keep = true;
    }
}

void ShadowAtlas::FreeUnusedEntries() {
    for (int index = 0; index < entries.Count();) {
        if (entries[index].keep) {
            entries[index].keep = false;
            ++index;
        } else {
            RemoveEntry(index);
        }
    }
}

ShadowAtlas::Entry *ShadowAtlas::Alloc(uint64_t lightId, uint32_t requestedResolution, uint32_t tileCount) {
    if (lightId == 0 || FindEntryIndex(lightId) >= 0) {
        return nullptr;
    }

    Entry entry;
    if (!allocator.Allocate(requestedResolution, tileCount, entry.tiles)) {
        return nullptr;
    }

    entry.lightId = lightId;
    entry.requestedResolution = requestedResolution;
    entry.numTiles = tileCount;
    entry.renderState = MakeShared<RenderState>();

    // entries 가 꽉차면 2배씩 용량을 증가시킨다.
    if (entries.Count() == entries.Capacity()) {
        entries.Reserve(BE1::Max(64, entries.Capacity() * 2));
    }

    const int index = entries.Append(entry);
    entryHash.Add(entryHash.GenerateHash(lightId), index);
    return &entries[index];
}

bool ShadowAtlas::Free(uint64_t lightId) {
    const int index = FindEntryIndex(lightId);
    if (index < 0) {
        return false;
    }

    RemoveEntry(index);
    return true;
}

void ShadowAtlas::RemoveEntry(int index) {
    assert(entries.IsValidIndex(index));
    const Entry &entry = entries[index];

    // 모든 타일 반환
    for (uint32_t tileIndex = 0; tileIndex < entry.numTiles; ++tileIndex) {
        const bool freed = allocator.Free(entry.tiles[tileIndex].handle);
        assert(freed);
        (void)freed;
    }

#if 1
    // 마지막 항목만 빈 자리로 옮기고 hash 를 갱신하여 나머지 항목의 이동을 피한다.
    entryHash.Remove(entryHash.GenerateHash(entries[index].lightId), index);
    const int lastIndex = entries.Count() - 1;
    if (index != lastIndex) {
        entryHash.Remove(entryHash.GenerateHash(entries[lastIndex].lightId), lastIndex);
    }
    entries[index].renderState.reset();
    entries.RemoveIndexFast(index);
    if (index != lastIndex) {
        entryHash.Add(entryHash.GenerateHash(entries[index].lightId), index);
    }
#else
    entryHash.RemoveIndex(entryHash.GenerateHash(entries[index].lightId), index);
    entries[index].renderState.reset();
    entries.RemoveIndex(index);
#endif
}
