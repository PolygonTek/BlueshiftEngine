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

class D3D12CommandList;
class D3D12Mesh;

enum class D3D12MeshType : byte {
    None,
    TriangleMesh,
    CubeMesh
};

class D3D12RenderObject {
public:
    struct State {
        D3D12MeshType       meshType;
        D3D12Mesh*          mesh = nullptr;
        Mat3x4              worldMatrix;
        Vec2                offset;
    };

    void                    Update(const State &state);

    void                    Draw(int threadIndex, int drawIndex, D3D12CommandList* commandList);

    void                    DrawTriangleMesh(int threadIndex, int drawIndex, D3D12CommandList* commandList);
    void                    DrawCubeMesh(int threadIndex, int drawIndex, D3D12CommandList* commandList);

    State                   state;      // 오브젝트를 렌더링할 때 필요한 실제 상태를 들고 있음 (Update 함수에서 갱신됨)
    int                     index = -1; // D3D12App::renderObjects 의 인덱스
};
