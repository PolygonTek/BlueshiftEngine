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

#include "RHI.h"

class Skeleton;

class SkinningJointCache {
public:
    SkinningJointCache() = delete;
    explicit SkinningJointCache(int numJoints);
    ~SkinningJointCache();

    void                    Purge();

    int                     NumJoints() const { return numJoints; }

    RHI::Buffer *           GetJointBuffer() const { return jointBuffer; }

    bool                    Update(RHI::FrameThreadData *frameThreadData, const Skeleton *skeleton, const BE1::Mat3x4 *jointMats);

private:
    int                     numJoints = 0;
    BE1::Mat3x4 *           skinningJoints = nullptr;    ///< jointMats * inverse bind-pose matrices.
    RHI::Buffer *           jointBuffer = nullptr;       ///< Frame dynamic structured buffer; not owned.
};
