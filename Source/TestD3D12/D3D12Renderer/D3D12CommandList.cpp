// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"

static constexpr D3D12_SHADING_RATE ToD3D12ShadingRate(RHIRenderer::ShadingRate shadingRate) {
    switch (shadingRate) {
    case RHIRenderer::ShadingRate::Rate1X1:
        return D3D12_SHADING_RATE_1X1;
    case RHIRenderer::ShadingRate::Rate1X2:
        return D3D12_SHADING_RATE_1X2;
    case RHIRenderer::ShadingRate::Rate2X1:
        return D3D12_SHADING_RATE_2X1;
    case RHIRenderer::ShadingRate::Rate2X2:
        return D3D12_SHADING_RATE_2X2;
    case RHIRenderer::ShadingRate::Rate2X4:
        return D3D12_SHADING_RATE_2X4;
    case RHIRenderer::ShadingRate::Rate4X2:
        return D3D12_SHADING_RATE_4X2;
    case RHIRenderer::ShadingRate::Rate4X4:
        return D3D12_SHADING_RATE_4X4;
    }
    return D3D12_SHADING_RATE_1X1;
}

int D3D12CommandList::GetThreadIndex() const {
    return parentPool->GetThreadIndex();
}

void D3D12CommandList::SetShadingRate(RHIRenderer::ShadingRate shadingRate) {
    D3D12_SHADING_RATE rate = ToD3D12ShadingRate(shadingRate);
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedShadingRate == rate) {
        return;
    }
    cachedShadingRate = rate;
#endif
    D3D12_SHADING_RATE_COMBINER combiners[] = {
        D3D12_SHADING_RATE_COMBINER_MAX,
        D3D12_SHADING_RATE_COMBINER_MAX
    };
    graphicsCommandList->RSSetShadingRate(rate, combiners);
}

void D3D12CommandList::Draw(uint32_t vertexCount, uint32_t startVertexLocation) {
    graphicsCommandList->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
}

void D3D12CommandList::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) {
    graphicsCommandList->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void D3D12CommandList::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
    graphicsCommandList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

void D3D12CommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) {
    graphicsCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
