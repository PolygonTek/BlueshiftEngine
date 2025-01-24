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

#pragma once

#include "D3D12Common.h"
#include "D3D12Buffer.h"

class D3D12Renderer;
class D3D12FrameThreadData;

// 상수 버퍼의 각 변수는 16 바이트 경계에 맞춰야 한다.
// 상수 버퍼 뷰의 GPU 주소와 크기는 256 바이트의 배수로 제한되어 있다.
// 상수 버퍼 뷰의 최대 크기는 64KB 이다.
class D3D12ConstantBuffer : public RHI::ConstantBuffer {
    friend class D3D12Renderer;
    friend class D3D12FrameThreadData;

public:
    virtual ~D3D12ConstantBuffer() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeBufferObject() const override { return buffer; }

    ID3D12Resource *                GetResource() const;

private:
    D3D12Buffer *                   buffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     descriptorHandle = {};
};
