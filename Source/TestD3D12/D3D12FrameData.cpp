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
#include "D3D12FrameData.h"
#include "D3D12Renderer.h"

void D3D12FrameData::Init() {
    // 그래픽스 커맨드 리스트를 위한 커맨드 할당자 생성
    HRESULT hr = renderer.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandAllocator : failed");
    }

    // 렌더링에 사용할 디스크립터 힙을 생성한다.
    // 최대 1000 개의 CBV_SRV_UAV 용 디스크립터를 담을 수 있다.
    rootDescriptorPool = new D3D12DescriptorPool;
    rootDescriptorPool->Init(1000);
}

void D3D12FrameData::Shutdown() {
    SAFE_DELETE(rootDescriptorPool);
    SAFE_RELEASE(commandAllocator);
}
