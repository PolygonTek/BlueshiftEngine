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

#include "../RHIRenderer.h"
#include "D3D12Common.h"
#include "D3D12Buffer.h"

class D3D12Renderer;
class D3D12FrameData;

class D3D12ConstantBuffer : public RHIRenderer::ConstantBuffer {
    friend class D3D12Renderer;
    friend class D3D12FrameData;

public:
    virtual ~D3D12ConstantBuffer() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const { return GetResource(); }

    ID3D12Resource *                GetResource() const;

private:
    D3D12Buffer *                   buffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     descriptorHandle = {};
};
