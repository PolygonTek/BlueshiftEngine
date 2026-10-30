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

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(ShaderProgram program, const BE1::StrArray &propertyDefines, const ShaderFeatureSet &featureSet) {
    if (program >= ShaderProgram::Count) {
        return Invalid();
    }

    const uint64_t programFeatures = (uint64_t(program) << 32) | uint64_t(featureSet.flags);
    uint64_t hash = 0;
    if (programFeatures != 0) {
        hash = hash_combine(hash, programFeatures);
    }

    for (const BE1::Str &define : propertyDefines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    // 계산된 hash 가 우연히 InvalidHash 와 같아지면, 실패 키와 구분되도록 다른 값으로 바꾼다.
    return ShaderPermutationKey(hash == InvalidHash ? InvalidHash - 1 : hash);
}

void ShaderPermutationKey::NormalizeDefines(BE1::StrArray &defines) {
    // Remove empty defines.
    for (int i = defines.Count() - 1; i >= 0; --i) {
        if (defines[i].IsEmpty()) {
            defines.RemoveIndexFast(i);
        }
    }

    const int numDefines = defines.Count();
    if (numDefines < 2) {
        return;
    }

    // Keep permutation keys deterministic.
    defines.Sort();

    // Remove duplicated defines by compacting in place.
    int writeIndex = 1;
    for (int readIndex = 1; readIndex < numDefines; readIndex++) {
        if (defines[writeIndex - 1] == defines[readIndex]) {
            continue;
        }

        if (writeIndex != readIndex) {
            defines[writeIndex] = std::move(defines[readIndex]);
        }
        writeIndex++;
    }
    defines.SetCount(writeIndex, false);
}
