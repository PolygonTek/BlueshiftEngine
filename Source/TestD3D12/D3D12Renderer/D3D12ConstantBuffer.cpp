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
#include "D3D12ConstantBuffer.h"
#include "D3D12DescriptorPool.h"
#include "D3D12CommandList.h"
#include "D3D12FrameThreadData.h"

void D3D12ConstantBuffer::Release() {
    if (!writePtr) {
        if (descriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(descriptorHandle);
            descriptorHandle.ptr = 0;
        }
    }
    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer, true);
        buffer = nullptr;
    }
}

RHI::ConstantBuffer* D3D12Renderer::CreateConstantBuffer(RHI::BufferUsage usage, uint32_t size, const void *data) {
    D3D12Buffer *buffer = static_cast<D3D12Buffer *>(CreateBuffer(usage, RHI::BufferFlag::ConstantBuffer, size, data));
    if (!buffer) {
        return nullptr;
    }

    // 상수 버퍼 뷰의 최대 크기는 64KB 이다.
    if (buffer->size > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_ERRLOG("D3D12Renderer::CreateConstantBuffer: The requested size of %i bytes exceeds the constant buffer maximum limit of 64KB\n", buffer->size);
        DestroyBuffer(buffer);
        return nullptr;
    }

    // 디스크립터 핸들 생성
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!resCpuDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        DestroyBuffer(buffer);
        return nullptr;
    }

    // 디스크립터 슬롯에 CBV 를 기록
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = buffer->GetResource()->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = buffer->size;
    device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

    // 상수 버퍼 객체 생성
    D3D12ConstantBuffer* constantBuffer = new D3D12ConstantBuffer;
    constantBuffer->buffer = buffer;
    constantBuffer->descriptorHandle = descriptorHandle;

    return constantBuffer;
}

void D3D12Renderer::DestroyConstantBuffer(RHI::ConstantBuffer *constantBuffer, bool immediate) {
    if (immediate) {
        delete constantBuffer;
    } else {
        DeleteGPUObject(constantBuffer);
    }
}

void D3D12Renderer::SetConstantBuffer(RHI::CommandList *commandList, int shaderRegister, const RHI::ConstantBuffer *constantBuffer) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    D3D12RootBindingData *rootBindingData = _commandList->GetRootBindingData();

    // 셰이더 레지스터에 대한 루트 파라미터 인덱스를 얻는다.
    const D3D12PipelineState::RegisterBinder &binder = _commandList->currentPSO->binder;
    assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.cbv));
    uint8_t rootParameterIndex = binder.rootParameterIndexLUT.cbv[shaderRegister];
    if (rootParameterIndex >= 64) {
        BE_ERRLOG("D3D12Renderer::SetConstantBuffer: Invalid root parameter index\n");
        return;
    }

    const D3D12ConstantBuffer *_constantBuffer = static_cast<const D3D12ConstantBuffer *>(constantBuffer);

    // 루트 파라미터가 디스크립터 테이블일 경우 테이블 내의 인덱스 LUT 에는 유효한 값이 들어있다.
    uint8_t indexInTable = binder.descriptorTableIndexLUT.cbv[shaderRegister];
    if (indexInTable != 0xFF) {
        // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
        rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable] = _constantBuffer->descriptorHandle;
    }
    // 셰이더 레지스터에 대한 상수 버퍼 객체도 저장한다.
    rootBindingData->cbvResources[shaderRegister] = _constantBuffer;

    // 변경할 루트 파라미터 인덱스에 대한 dirty mask 를 설정한다.
    if (_commandList->currentPSO->isGraphics) {
        _commandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        _commandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
