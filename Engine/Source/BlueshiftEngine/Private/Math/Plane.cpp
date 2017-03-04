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
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

int Plane::GetType() const {
    if (Normal()[0] == 0.0f) {
        if (Normal()[1] == 0.0f) {
            return Normal()[2] > 0.0f ? Type::Z : Type::NegZ;
        } else if (Normal()[2] == 0.0f) {
            return Normal()[1] > 0.0f ? Type::Y : Type::NegY;
        } else {
            return Type::ZeroX;
        }
    } else if (Normal()[1] == 0.0f) {
        if (Normal()[2] == 0.0f) {
            return Normal()[0] > 0.0f ? Type::X : Type::NegX;
        } else {
            return Type::ZeroY;
        }
    } else if (Normal()[2] == 0.0f) {
        return Type::ZeroZ;
    } else {
        return Type::NonAxial;
    }
}

const char *Plane::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 4, precision);
}

BE_NAMESPACE_END
