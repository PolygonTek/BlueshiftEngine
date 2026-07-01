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

#include "Precompiled.h"
#include "ShaderPermutationKey.h"

void ShaderPermutationKey::NormalizePermutationDefines(BE1::StrArray &permutationDefines) {
    // Remove empty defines.
    for (int i = permutationDefines.Count() - 1; i >= 0; --i) {
        if (permutationDefines[i].IsEmpty()) {
            permutationDefines.RemoveIndexFast(i);
        }
    }

    // Keep permutation keys deterministic.
    permutationDefines.Sort();

    // Remove duplicated defines.
    for (int i = 1; i < permutationDefines.Count();) {
        if (permutationDefines[i - 1] == permutationDefines[i]) {
            permutationDefines.RemoveIndex(i);
            continue;
        }
        i++;
    }
}

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(const BE1::StrArray &permutationDefines, const ShaderFeatureSet &featureSet) {
    if (permutationDefines.IsEmpty() && featureSet.IsEmpty()) {
        return ShaderPermutationKey();
    }

    uint64_t hash = 1469598103934665603ull;
    if (!featureSet.IsEmpty()) {
        hash = hash_combine(hash, uint64_t(featureSet.flags));
    }
    for (const BE1::Str &define : permutationDefines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    return ShaderPermutationKey(hash);
}
