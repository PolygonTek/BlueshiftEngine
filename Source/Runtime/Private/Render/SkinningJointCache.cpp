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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Heap.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

SkinningJointCache::SkinningJointCache(int numJoints) {
    this->skinningJoints = nullptr;
    this->jointIndexOffset[0] = 0;
    this->jointIndexOffset[1] = 0;
    this->viewFrameCount = -1;
    this->numJoints = numJoints;

    // NOTE: VTF skinning 일 때만 모션블러 함
    if (renderGlobal.skinningMethod == SkinningJointCache::VertexTextureFetchSkinning) {
        if (r_motionBlur.GetInteger() == 2) {
            this->numJoints *= 2;
        }
        this->skinningJoints = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4) * this->numJoints);
    } else {
        this->skinningJoints = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4) * this->numJoints);
    }
}

void SkinningJointCache::Purge() {
    if (skinningJoints) {
        Mem_AlignedFree(skinningJoints);
        skinningJoints = nullptr;
    }
}

void SkinningJointCache::Update(const Skeleton *skeleton, const Mat3x4 *jointMats) {
    if (viewFrameCount == renderSystem.GetCurrentRenderContext()->frameCount) {
        return;
    }

    viewFrameCount = renderSystem.GetCurrentRenderContext()->frameCount;

    if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
        if (viewFrameCount == renderSystem.GetCurrentRenderContext()->frameCount) {
            jointIndexOffset[1] = jointIndexOffset[0];
            jointIndexOffset[0] = jointIndexOffset[0] == 0 ? numJoints : 0;
        }
    } else {
        jointIndexOffset[0] = 0;
        jointIndexOffset[1] = 0;
    }

    simdProcessor->MultiplyJoints(skinningJoints + jointIndexOffset[0], jointMats, skeleton->GetInvBindPoseMatrices(), numJoints);

    if (renderGlobal.skinningMethod == SkinningJointCache::VertexTextureFetchSkinning) {
        bufferCacheManager.AllocTexel(numJoints * sizeof(Mat3x4), skinningJoints, &bufferCache);
    }
}

bool SkinningJointCache::CapableGPUJointSkinning(SkinningMethod skinningMethod, int numJoints) {
    assert(numJoints > 0 && numJoints < 256);

    if (skinningMethod == SkinningJointCache::VertexTextureFetchSkinning) {
        return true;
    } else if (skinningMethod == SkinningJointCache::VertexShaderSkinning) {
        if (numJoints <= 74) {
            if (rhi.HWLimit().maxVertexUniformComponents >= 256) {
                return true;
            }
        } else if (numJoints <= 256) {
            if (rhi.HWLimit().maxVertexUniformComponents >= 2048) {
                return true;
            }
        }
    }

    return false;
}

BE_NAMESPACE_END
