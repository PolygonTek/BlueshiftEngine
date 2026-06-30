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
#include "SkeletonManager.h"
#include "Core/Cmds.h"

SkeletonManager     skeletonManager;

void SkeletonManager::Init() {
    BE1::cmdSystem.AddCommand("listSkeletons", Cmd_ListSkeletons);
    BE1::cmdSystem.AddCommand("reloadSkeleton", Cmd_ReloadSkeleton);

    skeletonHashMap.Init(1024, 64, 64);

    // Create default skeleton
    defaultSkeleton = AllocSkeleton("_defaultSkeleton");
    defaultSkeleton->CreateDefaultSkeleton();
}

void SkeletonManager::Shutdown() {
    BE1::cmdSystem.RemoveCommand("listSkeletons");
    BE1::cmdSystem.RemoveCommand("reloadSkeleton");

    defaultSkeleton.reset();

    skeletonHashMap.Clear();
}

void SkeletonManager::UpdateFrame() {
    CleanExpiredSkeletons();
}

SharedPtr<Skeleton> SkeletonManager::AllocSkeleton(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("SkeletonManager::AllocSkeleton: empty skeleton name");
    }
    if (const auto *entry = skeletonHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("SkeletonManager::AllocSkeleton: %s skeleton already allocated", hashName);
        }
        skeletonHashMap.Remove(hashName);
    }

    SharedPtr<Skeleton> skeleton = MakeShared<Skeleton>();
    skeleton->hashName = hashName;
    skeleton->name = hashName;
    skeleton->name.StripPath();
    skeleton->name.StripFileExtension();
    skeletonHashMap.Set(skeleton->hashName, skeleton);

    return skeleton;
}

SharedPtr<Skeleton> SkeletonManager::FindSkeleton(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = skeletonHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

SharedPtr<Skeleton> SkeletonManager::GetSkeleton(const char *hashName, bool async) {
    if (!hashName || !hashName[0]) {
        return defaultSkeleton;
    }

    SharedPtr<Skeleton> skeleton = FindSkeleton(hashName);
    if (skeleton) {
        if (!async && !skeleton->WaitReady()) {
            return defaultSkeleton;
        }
        return skeleton;
    }

    skeleton = AllocSkeleton(hashName);
#ifdef USE_ASYNC_LOADING
    bool loaded = async ? skeleton->LoadAsync(hashName) : skeleton->Load(hashName);
#else
    bool loaded = skeleton->Load(hashName);
#endif
    if (!loaded) {
        skeletonHashMap.Remove(skeleton->hashName);
        return defaultSkeleton;
    }

    return skeleton;
}

void SkeletonManager::RenameSkeleton(Skeleton *skeleton, const BE1::Str &newName) {
    /*const auto *entry = skeletonHashMap.Get(skeleton->hashName);
    if (entry) {
        skeletonHashMap.Remove(skeleton->hashName);

        skeleton->hashName = newName;
        skeleton->name = newName;
        skeleton->name.StripPath();
        skeleton->name.StripFileExtension();

        skeletonHashMap.Set(newName, skeleton);
    }*/
}

void SkeletonManager::CleanExpiredSkeletons() {
    constexpr int ExpiredSkeletonHashMapScanBudget = 32;
    int scansLeft = ExpiredSkeletonHashMapScanBudget;

    while (scansLeft-- > 0 && skeletonHashMap.Count() > 0) {
        if (expiredSkeletonHashMapCursor >= skeletonHashMap.Count()) {
            expiredSkeletonHashMapCursor = 0;
        }

        const auto *entry = skeletonHashMap.GetByIndex(expiredSkeletonHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            skeletonHashMap.Remove(entry->first);
            continue;
        }

        expiredSkeletonHashMapCursor++;
    }
}

//--------------------------------------------------------------------------------------------------

void SkeletonManager::Cmd_ListSkeletons(const BE1::CmdArgs &args) {
    int count = 0;

    const int numSkeletons = skeletonManager.skeletonHashMap.Count();
    for (int i = 0; i < numSkeletons; i++) {
        const auto *entry = skeletonManager.skeletonHashMap.GetByIndex(i);
        SharedPtr<Skeleton> skeleton = entry->second.lock();
        if (!skeleton) {
            continue;
        }

        BE_LOG("%3d refs %3d joints : %s\n",
            skeleton.use_count(),
            skeleton->NumJoints(),
            skeleton->GetHashName());

        count++;
    }

    BE_LOG("%i total skeletones\n", count);
}

void SkeletonManager::Cmd_ReloadSkeleton(const BE1::CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("reloadSkeleton <filename>\n");
        return;
    }

    if (!BE1::Str::Icmp(args.Argv(1), "all")) {
        for (const auto &entry : skeletonManager.skeletonHashMap) {
            SharedPtr<Skeleton> skeleton = entry.second.lock();
            if (skeleton) {
                skeleton->Reload();
            }
        }
    } else {
        SharedPtr<Skeleton> skeleton = skeletonManager.FindSkeleton(args.Argv(1));
        if (!skeleton) {
            BE_WARNLOG("Couldn't find skeleton to reload \"%s\"\n", args.Argv(1));
            return;
        }

        skeleton->Reload();
    }
}
