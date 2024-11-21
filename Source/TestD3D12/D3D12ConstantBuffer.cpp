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
#include "D3D12ConstantBuffer.h"
#include "D3D12Buffer.h"
#include "D3D12Renderer.h"

void D3D12ConstantBuffer::Release() {
    SAFE_DELETE(buffer);
}

void *D3D12ConstantBuffer::Map(SIZE_T begin, SIZE_T end) {
    void* mappedPtr = nullptr;
    CD3DX12_RANGE range(0, 0); // We do not intend to read from this resource on the CPU.
    buffer->GetResource()->Map(0, &range, reinterpret_cast<void **>(&mappedPtr));
    return mappedPtr;
}

void D3D12ConstantBuffer::Unmap() {
    buffer->GetResource()->Unmap(0, nullptr);
}

D3D12ConstantBuffer* D3D12ConstantBuffer::CreateConstantBuffer(int size) {
    // 상수 버퍼는 어차피 GPU 에 요청하면 256 바이트로 주소 & 사이즈가 정렬된다.
    UINT alignedSize = (UINT)AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    D3D12Buffer *buffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Upload, alignedSize);
    if (!buffer) {
        return nullptr;
    }

    D3D12ConstantBuffer* constantBuffer = new D3D12ConstantBuffer;
    constantBuffer->buffer = buffer;

    return constantBuffer;
}
