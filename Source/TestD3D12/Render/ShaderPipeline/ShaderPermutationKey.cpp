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

void ShaderPermutationKey::NormalizeDefines(BE1::StrArray &defines) {
    // Remove empty defines.
    for (int i = defines.Count() - 1; i >= 0; --i) {
        if (defines[i].IsEmpty()) {
            defines.RemoveIndexFast(i);
        }
    }

    // Keep permutation keys deterministic.
    defines.Sort();

    // Remove duplicated defines.
    for (int i = 1; i < defines.Count();) {
        if (defines[i - 1] == defines[i]) {
            defines.RemoveIndex(i);
            continue;
        }
        i++;
    }
}

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(const BE1::StrArray &defines, const ShaderFeatureSet &featureSet) {
    if (defines.IsEmpty() && featureSet.IsEmpty()) {
        return ShaderPermutationKey();
    }

    uint64_t hash = 1469598103934665603ull;
    if (!featureSet.IsEmpty()) {
        hash = hash_combine(hash, uint64_t(featureSet.flags));
    }
    for (const BE1::Str &define : defines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    return ShaderPermutationKey(hash);
}
