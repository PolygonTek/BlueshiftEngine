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

#pragma once

#include "D3D12Common.h"

/*
---------------------------------------------------------------------------------------------------

    Root Binding Data

    - 셰이더 레지스터 별로 바인딩하려는 루트 파라미터 별 리소스를 임시 저장하는 객체
        - 디스크립터 테이블
        - 루트 상수값
        - 리소스 포인터
    - 메인 스레드 혹은 프레임 데이터의 워커 스레드 별로 하나씩 존재한다. (커맨드 리스트 풀과 1:1 대응)

---------------------------------------------------------------------------------------------------
*/

struct D3D12RootBindingData {
    static constexpr uint32_t           MaxRootParameters = 64;
    static constexpr uint32_t           MaxDescriptorsInDescriptorTable = 64;
    static constexpr uint32_t           MaxRootConstantRegisters = 4;
    static constexpr uint32_t           MaxRootConstant32BitValues = 16;

                                        // 루트 파라미터 별 디스크립터 테이블에서 사용할 디스크립터들의 CPU 핸들
    D3D12_CPU_DESCRIPTOR_HANDLE         tableDescriptorHandles[MaxRootParameters][MaxDescriptorsInDescriptorTable] = { CD3DX12_CPU_DESCRIPTOR_HANDLE() };

                                        // 루트 상수의 셰이더 레지스터 별 32 비트 값의 배열
    uint32_t                            rootConstants[MaxRootConstantRegisters][MaxRootConstant32BitValues] = {};

                                        // 셰이더 레지스터 별 CBV GPU 주소
    D3D12_GPU_VIRTUAL_ADDRESS           cbvGpuAddresses[16] = {};
                                        // 셰이더 레지스터 별 SRV GPU 주소
    D3D12_GPU_VIRTUAL_ADDRESS           srvGpuAddresses[64] = {};
                                        // 셰이더 레지스터 별 UAV GPU 주소
    D3D12_GPU_VIRTUAL_ADDRESS           uavGpuAddresses[8] = {};
};
