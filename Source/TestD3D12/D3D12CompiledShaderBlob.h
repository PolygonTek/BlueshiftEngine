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

class D3D12CompiledShaderBlob : public ID3DBlob {
public:
    explicit D3D12CompiledShaderBlob(size_t dataSize);
    D3D12CompiledShaderBlob(const void *data, size_t dataSize);
    ~D3D12CompiledShaderBlob();

    virtual ULONG STDMETHODCALLTYPE     AddRef() override;
    virtual ULONG STDMETHODCALLTYPE     Release() override;
    virtual HRESULT STDMETHODCALLTYPE   QueryInterface(REFIID riid, void **ppvObject) override;
    virtual void *STDMETHODCALLTYPE     GetBufferPointer() override { return (byte *)buffer + sizeof(uint32_t); }
    virtual SIZE_T STDMETHODCALLTYPE    GetBufferSize() override { return size - sizeof(uint32_t); }

private:
    std::atomic_ulong                   refCount;
    void *                              buffer = nullptr;
    size_t                              size = 0;
};

BE_INLINE D3D12CompiledShaderBlob::D3D12CompiledShaderBlob(size_t dataSize)
    : refCount(1), size(dataSize) {
    buffer = Mem_Alloc32(dataSize);
}

BE_INLINE D3D12CompiledShaderBlob::D3D12CompiledShaderBlob(const void *data, size_t dataSize)
    : refCount(1), size(dataSize) {
    buffer = Mem_Alloc32(dataSize);
    memcpy(buffer, data, dataSize);
}

BE_INLINE D3D12CompiledShaderBlob::~D3D12CompiledShaderBlob() {
    if (buffer) {
        Mem_AlignedFree(buffer);
        buffer = nullptr;
    }
}
