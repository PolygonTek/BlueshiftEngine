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
#include "D3D12Renderer.h"
#include "D3D12Query.h"
#include "D3D12CommandList.h"

static constexpr D3D12_QUERY_TYPE ToD3D12QueryType(RHI::QueryType queryType) {
    switch (queryType) {
    case RHI::QueryType::TimeStamp:
        return D3D12_QUERY_TYPE_TIMESTAMP;
    case RHI::QueryType::OcclusionBinary:
        return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
    case RHI::QueryType::Occlusion:
        return D3D12_QUERY_TYPE_OCCLUSION;
    }
    assert(0);
    return D3D12_QUERY_TYPE_TIMESTAMP;
}

RHI::QueryHeap* D3D12Renderer::CreateQueryHeap(const RHI::QueryHeapDesc *desc) {
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Count = desc->queryCount;

    switch (desc->type) {
    case RHI::QueryType::TimeStamp:
        queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        break;
    case RHI::QueryType::Occlusion:
    case RHI::QueryType::OcclusionBinary:
        queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        break;
    }

    ID3D12QueryHeap *internalQueryHeap = nullptr;
    HRESULT hr = device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&internalQueryHeap));
    if (FAILED(hr)) {
        return nullptr;
    }

    D3D12QueryHeap *queryHeap = new D3D12QueryHeap;
    queryHeap->desc = *desc;
    queryHeap->queryHeap = internalQueryHeap;
    return queryHeap;
}

void D3D12Renderer::DestroyQueryHeap(RHI::QueryHeap *queryHeap, bool immediate) {
    if (immediate) {
        delete queryHeap;
    } else {
        MarkForDelete(queryHeap);
    }
}

void D3D12Renderer::BeginQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *d3d12QueryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);

    d3d12CommandList->GetGraphicsCommandList()->BeginQuery(d3d12QueryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index);
}

void D3D12Renderer::EndQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *d3d12QueryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);

    d3d12CommandList->GetGraphicsCommandList()->EndQuery(d3d12QueryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index);
}

void D3D12Renderer::ResolveQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count, const RHI::Buffer *destBuffer, uint64_t destOffset) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *d3d12QueryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);
    const D3D12Buffer *d3d12Buffer = static_cast<const D3D12Buffer *>(destBuffer);

    d3d12CommandList->GetGraphicsCommandList()->ResolveQueryData(d3d12QueryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index, count, d3d12Buffer->GetResource(), destOffset);
}

void D3D12Renderer::ResetQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count) {
}
