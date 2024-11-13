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

#include "D3D12FrameData.h"

class D3D12DescriptorPool;

class D3D12FrameData {
public:
    void                            Init();
    void                            Shutdown();

    ID3D12CommandAllocator*         commandAllocator = nullptr;
    ID3D12GraphicsCommandList*      commandList = nullptr;
    D3D12DescriptorPool *           rootDescriptorPool = nullptr;
    UINT64                          fenceValue = 0;
};
