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

    ShaderPermutationKey

    * permutation key 는 permutation defines (shader define properties) + featureSet flags 으로 만들어 진다.

---------------------------------------------------------------------------------------------------
*/

struct ShaderPermutationKey {
    ShaderPermutationKey() = default;
    explicit ShaderPermutationKey(uint64_t hash) : hash(hash) {}

    bool                            operator==(const ShaderPermutationKey &rhs) const { return hash == rhs.hash; }
    bool                            operator!=(const ShaderPermutationKey &rhs) const { return hash != rhs.hash; }

    static void                     NormalizePermutationDefines(BE1::StrArray &permutationDefines);

    static ShaderPermutationKey     GeneratePermutationKey(const BE1::StrArray &permutationDefines, const ShaderFeatureSet &featureSet = {});

    uint64_t                        hash = 0;
};
