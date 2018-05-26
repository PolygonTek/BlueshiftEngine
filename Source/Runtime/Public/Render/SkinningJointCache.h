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

#pragma once

BE_NAMESPACE_BEGIN

class Mat3x4;
class Skeleton;
class Batch;

// Joint cache for HW skinning
class SkinningJointCache {
    friend class Batch;

public:
    enum SkinningMethod {
        CpuSkinning,
        VertexShaderSkinning,
        VertexTextureFetchSkinning
    };

    SkinningJointCache() = delete;
    SkinningJointCache(int numJoints);
    ~SkinningJointCache();

    void                Purge();

    const BufferCache & GetBufferCache() const { return bufferCache; }

    void                Update(const Skeleton *skeleton, const Mat3x4 *jointMats);

    static bool         CapableGPUJointSkinning(SkinningMethod skinningMethod, int numJoints);

private:
    int                 numJoints;              // motion blur 를 사용하면 원래 model joints 의 2배를 사용한다
    Mat3x4 *            skinningJoints;         // result matrix for animation
    int                 jointIndexOffset[2];    // current/previous frame joint index offset for motion blur
    BufferCache         bufferCache;            // use for VTF skinning
    int                 viewFrameCount;         // 현재 프레임에 계산을 마쳤음을 표시하기 위한 marking number
};

BE_INLINE SkinningJointCache::~SkinningJointCache() {
    Purge();
}

BE_NAMESPACE_END
