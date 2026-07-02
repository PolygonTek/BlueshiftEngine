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
#include "AnimManager.h"
#include "LoadingSystem.h"
#include "Core/Cmds.h"

AnimManager     animManager;

void AnimManager::Init() {
    //BE1::cmdSystem.AddCommand("listAnims", Cmd_ListAnims);

    animHashMap.Init(1024, 64, 64);

    expiredAnimHashMapCursor = 0;
}

void AnimManager::Shutdown() {
    //BE1::cmdSystem.RemoveCommand("listAnims");

    animHashMap.Clear();

    jointNames.Clear();
    jointNameHash.Free();
}

void AnimManager::UpdateFrame() {
    CleanExpiredAnims();
}

SharedPtr<Anim> AnimManager::AllocAnim(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("AnimManager::AllocAnim: empty anim name");
    }

    if (const auto *entry = animHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("AnimManager::AllocAnim: %s anim already allocated", hashName);
        }
        animHashMap.Remove(hashName);
    }

    SharedPtr<Anim> anim = MakeShared<Anim>();
    anim->hashName = hashName;
    anim->name = hashName;
    anim->name.StripPath();
    anim->name.StripFileExtension();

    animHashMap.Set(anim->hashName, anim);
    return anim;
}

SharedPtr<Anim> AnimManager::FindAnim(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = animHashMap.Get(BE1::Str(hashName));
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

SharedPtr<Anim> AnimManager::GetAnim(const char *hashName, bool async) {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const bool useAsync = async && LoadingSystem::IsAsyncAllowed();

    SharedPtr<Anim> anim = FindAnim(hashName);
    if (anim) {
        if (!useAsync && !anim->WaitReady()) {
            return nullptr;
        }
        return anim;
    }

    anim = AllocAnim(hashName);
    bool loaded = useAsync ? anim->LoadAsync(hashName) : anim->Load(hashName);
    if (!loaded) {
        BE_WARNLOG("Couldn't load anim '%s'\n", hashName);
        animHashMap.Remove(anim->hashName);
        anim = nullptr;
    }

    return anim;
}

SharedPtr<Anim> AnimManager::GetDefaultAnim(const char *hashName, const Skeleton *skeleton) {
    if (!hashName || !hashName[0] || !skeleton) {
        return nullptr;
    }

    SharedPtr<Anim> anim = FindAnim(hashName);
    if (anim) {
        return anim;
    }

    anim = AllocAnim(hashName);
    anim->CreateDefaultAnim(skeleton);

    return anim;
}

int	AnimManager::JointIndexByName(const char *jointName) {
    int index;
    int hash = jointNameHash.GenerateHash(jointName);
    for (index = jointNameHash.First(hash); index != -1; index = jointNameHash.Next(index)) {
        if (jointNames[index].Cmp(jointName) == 0) {
            return index;
        }
    }

    index = jointNames.Append(jointName);
    jointNameHash.Add(hash, index);
    return index;
}

const char *AnimManager::JointNameByIndex(int index) const {
    if (!jointNames.IsValidIndex(index)) {
        return "";
    }
    return jointNames[index];
}

void AnimManager::CleanExpiredAnims() {
    constexpr int ExpiredAnimHashMapScanBudget = 32;
    int scansLeft = ExpiredAnimHashMapScanBudget;

    while (scansLeft-- > 0 && animHashMap.Count() > 0) {
        if (expiredAnimHashMapCursor >= animHashMap.Count()) {
            expiredAnimHashMapCursor = 0;
        }

        const auto *entry = animHashMap.GetByIndex(expiredAnimHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            animHashMap.Remove(entry->first);
            continue;
        }

        expiredAnimHashMapCursor++;
    }
}
