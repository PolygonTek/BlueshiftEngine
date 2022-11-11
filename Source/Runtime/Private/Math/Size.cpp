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

const Size Size::zero = Size(0, 0);

SizeF Size::ToSizeF() const { 
    return SizeF(w, h); 
}

Size Size::FromString(const char *str) {
    Size s;
    int count = sscanf(str, "%i %i", &s.w, &s.h);
    assert(count == s.GetDimension());
    return s;
}

BE_NAMESPACE_END
