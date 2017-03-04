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

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

struct Vec4Color {
    // [0] LIGHT GRAY
    // [1] RED
    // [2] GREEN
    // [3] YELLOW
    // [4] BLUE
    // [5] CYAN
    // [6] MAGENTA
    // [7] WHITE
    // [8] GRAY
    // [9] BLACK
    // [10] ORANGE
    // [11] PINK
    // [12] LAWN
    // [13] MINT
    // [14] VIOLET
    // [15] TEAL
    static const Color4       table[16];
};

BE_NAMESPACE_END
