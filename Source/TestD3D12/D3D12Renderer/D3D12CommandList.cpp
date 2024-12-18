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

int D3D12CommandList::GetThreadIndex() const {
    return parentPool->GetThreadIndex();
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
