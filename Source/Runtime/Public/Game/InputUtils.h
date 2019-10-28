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

#include "Core/Guid.h"
#include "Math/Point.h"
#include "Containers/HashTable.h"

BE_NAMESPACE_BEGIN

class Entity;

struct InputUtils {
    struct PointerState {
        Guid oldHitEntityGuid;
        Guid captureEntityGuid;
    };

    /// Returns whether pointer input is processed or not.
    static bool ProcessMousePointerInput(PointerState &mousePointerState, const std::function<Entity *(const Point &)> &rayCastFunc);

    /// Returns whether pointer input is processed or not.
    static bool ProcessTouchPointerInput(HashTable<int32_t, PointerState> &touchPointerStateTable, const std::function<Entity *(const Point &)> &rayCastFunc);
};

BE_NAMESPACE_END
