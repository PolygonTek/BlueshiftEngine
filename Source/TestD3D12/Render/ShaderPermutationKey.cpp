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

void ShaderPermutationKey::NormalizePermutationDefines(BE1::StrArray &defines) {
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

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(const BE1::StrArray &defines) {
    if (defines.Count() == 0) {
        return ShaderPermutationKey();
    }

    uint64_t hash = 1469598103934665603ull;
    for (const BE1::Str &define : defines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    return ShaderPermutationKey(hash);
}

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(const BE1::StrArray &defines, const ShaderFeatureSet &featureSet) {
    if (featureSet.IsEmpty()) {
        return GeneratePermutationKey(defines);
    }

    uint64_t hash = 1469598103934665603ull;
    hash = hash_combine(hash, uint64_t(featureSet.flags));
    for (const BE1::Str &define : defines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    return ShaderPermutationKey(hash);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(std::initializer_list<const char *> defines) {
    BE1::StrArray permutationDefines;
    for (const char *defineName : defines) {
        if (defineName && defineName[0]) {
            permutationDefines.Append(defineName);
        }
    }

    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(const BE1::StrArray &defines) {
    BE1::StrArray permutationDefines = defines;
    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(std::initializer_list<const char *> defines, const ShaderFeatureSet &featureSet) {
    BE1::StrArray permutationDefines;
    for (const char *defineName : defines) {
        if (defineName && defineName[0]) {
            permutationDefines.Append(defineName);
        }
    }

    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines, featureSet);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(const BE1::StrArray &defines, const ShaderFeatureSet &featureSet) {
    BE1::StrArray permutationDefines = defines;
    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines, featureSet);
}
