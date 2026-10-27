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

#include <stdint.h>
#include "Containers/StrArray.h"
#include "ShaderFeatureSet.h"

/*
---------------------------------------------------------------------------------------------------

    Shader Permutation Key

    64 비트 해시 키는 property defines + featureSet flags 로 만들어진다.

---------------------------------------------------------------------------------------------------
*/

struct ShaderPermutationKey {
    ShaderPermutationKey() = default;
    explicit ShaderPermutationKey(uint64_t hash) : hash(hash) {}

    bool                            operator==(const ShaderPermutationKey &rhs) const { return hash == rhs.hash; }
    bool                            operator!=(const ShaderPermutationKey &rhs) const { return hash != rhs.hash; }

    static ShaderPermutationKey     GeneratePermutationKey(const BE1::StrArray &propertyDefines, const ShaderFeatureSet &featureSet = {});

                                    // 중복 define 제거 + deterministic 하게 순서 정렬
    static void                     NormalizeDefines(BE1::StrArray &defines);

    uint64_t                        hash = 0;
};
