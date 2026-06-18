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

class D3D12CommandList;
class D3D12RootBindingData;

/*
---------------------------------------------------------------------------------------------------

    Command List Pool

    - 같은 타입의 커맨드 리스트들을 미리 생성하여 보관한다.
    - 풀에서 커맨드 리스트를 할당받아 사용한 후, 다시 풀에 반환하여 재사용한다.
    - 같은 풀에서 병렬로 커맨드 리스트를 할당/반환할 수 없다. (not thread-safe)
    - 같은 풀은 한 스레드에서 순차적으로 사용하는 것을 전제로 한다. (멀티 스레드에서 동시에 같은 풀에서 커맨드 리스트를 할당/반환하는 것은 허용하지 않음)
    - 커멘드 리스트를 제출한 후 풀에서 다음 커맨드 리스트를 할당받아 순차적으로 사용할 수 있다.
    - 커맨드 리스트 풀은 D3D12RootBindingData 객체를 참조하여 할당한 커맨드 리스트들이 하나의 바인딩 데이터를 공유하도록 한다.

---------------------------------------------------------------------------------------------------
*/

class D3D12CommandListPool {
    friend class D3D12CommandList;

public:
    D3D12CommandListPool() = default;
    D3D12CommandListPool(D3D12RootBindingData *rootBindingData, D3D12_COMMAND_LIST_TYPE commandListType, uint32_t maxPrimaryCommandLists, uint32_t maxSecondaryCommandLists = 0) {
        Init(rootBindingData, commandListType, maxPrimaryCommandLists, maxSecondaryCommandLists);
    }
    ~D3D12CommandListPool() { Shutdown(); }

    void                            Init(D3D12RootBindingData *rootBindingData, D3D12_COMMAND_LIST_TYPE commandListType, uint32_t maxPrimaryCommandLists, uint32_t maxSecondaryCommandLists = 0);
    void                            Shutdown();

    D3D12RootBindingData *          GetRootBindingData() const { return rootBindingData; }

    D3D12_COMMAND_LIST_TYPE         GetCommandListType() const { return commandListType; }

    void                            Clear();

    D3D12CommandList *              Alloc(RHI::CommandListKind kind = RHI::CommandListKind::Primary);
    void                            Free(D3D12CommandList *commandList);

    int                             UsedCount() const { return usedCount; }

private:
    D3D12RootBindingData *          rootBindingData = nullptr;
    BE1::LinkList<D3D12CommandList> freeCommandLists[to_int(RHI::CommandListKind::Count)];
    D3D12CommandList *              commandListPool = nullptr;
    D3D12_COMMAND_LIST_TYPE         commandListType = D3D12_COMMAND_LIST_TYPE_NONE;
    uint32_t                        maxCommandLists = 0;
    int                             usedCount = 0;
};
