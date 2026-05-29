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
    case RHI::QueryType::Timestamp:
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
    case RHI::QueryType::Timestamp:
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
        DeleteGPUObject(queryHeap);
    }
}

void D3D12Renderer::BeginQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *_queryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);

    // Timestamp 는 BeginQuery 를 사용하지 않고, EndQuery 만 사용한다.
    assert(queryHeap->desc.type != RHI::QueryType::Timestamp);

    _commandList->GetGraphicsCommandList()->BeginQuery(_queryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index);
}

void D3D12Renderer::EndQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *_queryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);

    _commandList->GetGraphicsCommandList()->EndQuery(_queryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index);
}

void D3D12Renderer::ResolveQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count, const RHI::Buffer *destBuffer, uint64_t destOffset) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12QueryHeap *_queryHeap = static_cast<const D3D12QueryHeap *>(queryHeap);
    const D3D12Buffer *_buffer = static_cast<const D3D12Buffer *>(destBuffer);

    _commandList->GetGraphicsCommandList()->ResolveQueryData(_queryHeap->queryHeap, ToD3D12QueryType(queryHeap->desc.type), index, count, _buffer->GetResource(), destOffset);
}

void D3D12Renderer::ResetQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count) {
}

uint64_t D3D12Renderer::GetTimestampFrequency(RHI::CommandQueueType queueType) const {
    UINT64 frequency = 0;
    commandQueues[to_int(queueType)]->GetTimestampFrequency(&frequency);
    return frequency;
}
