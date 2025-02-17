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

class SubMesh;
class VisObject;
class Texture;

class DrawSurf {
public:
    enum class Flag : uint8_t {
        None                = 0,
        Visible             = BIT(0),           ///< Surface is visible by camera
        ShadowVisible       = BIT(1),           ///< Shadow of surface is visible by camera
        ShowWires           = BIT(2),
        SkipSelection       = BIT(3),
        UseInstancing       = BIT(4)
    };

    uint64_t                sortKey;
    SubMesh *               subMesh = nullptr;
    const VisObject *       space = nullptr;
    const Texture *         texture = nullptr;
    Flag                    flags = Flag::None;
};

template<>
struct enable_bitmask_operators<DrawSurf::Flag> {
    static const bool enable = true;
};
