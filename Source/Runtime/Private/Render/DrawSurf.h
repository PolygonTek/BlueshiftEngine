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

#include "Render/SubMesh.h"

BE_NAMESPACE_BEGIN

class DrawSurf {
public:
    struct Flag {
        enum Enum {
            Visible         = BIT(0),           ///< Surface is visible by camera
            ShadowVisible   = BIT(1),           ///< Shadow of surface is visible by camera
            ShowWires       = BIT(2),
            SkipSelection   = BIT(3),
            UseInstancing   = BIT(4)
        };
    };

                            /// 같은 SubMesh 면서 선택한 LOD 도 같은지 검사한다.
    bool                    IsSameGeometry(const DrawSurf *other) const { return subMesh->IsShared(other->subMesh) && lodLevel == other->lodLevel; }

    uint64_t                sortKey;
    uint32_t                flags;
    int                     lodLevel;           ///< 해당 뷰에서 사용할 LOD
    SubMesh *               subMesh;
    const BufferCache *     indexCache;         ///< 해당 LOD 의 공유 GPU 인덱스 캐시
    const VisObject *       space;              ///< Parent of this surface
    const Material *        material;           ///< Material of this surface
    const float *           materialRegisters;
    int                     numIndexes;         ///< 해당 LOD 의 인덱스 개수
    int                     instanceIndex;
};

BE_NAMESPACE_END
