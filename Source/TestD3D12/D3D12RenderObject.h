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

class D3D12Mesh;

struct D3D12MeshType {
    enum Enum {
        None,
        TriangleMesh,
        CubeMesh
    };
};

class D3D12RenderObject {
public:
    D3D12RenderObject(int index);

    struct State {
        D3D12MeshType::Enum meshType;
        D3D12Mesh*          mesh = nullptr;
        Mat3x4              worldMatrix;
        Vec2                offset;
    };

    void                    Update(const State &state);

    void                    Render(int renderIndex);

    void                    DrawTriangleMesh(int renderIndex);
    void                    DrawCubeMesh(int renderIndex);

    State                   state;
    int                     index = -1;
};

BE_INLINE D3D12RenderObject::D3D12RenderObject(int index) {
    this->index = index;
}
