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

#include "Math/Vector.h"

struct DebugDrawVertex {
    BE1::Vec3 xyz;
    uint32_t color = 0;
};

static_assert(sizeof(DebugDrawVertex) == sizeof(float) * 3 + sizeof(uint32_t));
static_assert(OFFSET_OF(DebugDrawVertex, color) == sizeof(float) * 3);
