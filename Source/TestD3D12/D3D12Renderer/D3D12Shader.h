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

class D3D12Renderer;

class D3D12Shader : public RHI::Shader {
    friend class D3D12Renderer;

public:
    virtual ~D3D12Shader() { Release(); }

    void                            Release();

private:
    uint64_t                        hash = 0;
    byte *                          compiledShaderData = nullptr;
    uint32_t                        compiledShaderDataSize = 0;
    ID3D12RootSignature *           rootSignature = nullptr;
    ID3D12VersionedRootSignatureDeserializer *rootSignatureDeserializer = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;
};
