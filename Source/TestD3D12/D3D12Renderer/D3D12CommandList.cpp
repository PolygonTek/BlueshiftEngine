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
#include "D3D12CommandList.h"

D3D12_PRIMITIVE_TOPOLOGY D3D12CommandList::ToD3D12PrimitiveTopology(RHIRenderer::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
    case RHIRenderer::PrimitiveTopology::PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RHIRenderer::PrimitiveTopology::LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RHIRenderer::PrimitiveTopology::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RHIRenderer::PrimitiveTopology::TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RHIRenderer::PrimitiveTopology::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
