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
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

AnimManager     animManager;

void AnimManager::Init() {
    cmdSystem.AddCommand(L"listAnims", Cmd_ListAnims);
}

void AnimManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listAnims");
        
    animHashMap.DeleteContents(true);
    
    jointNameList.Clear();
    jointNameHash.Free();
}

Anim *AnimManager::AllocAnim(const char *hashName) {
    if (animHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs anim already allocated", hashName);
    }
    
    Anim *anim = new Anim;
    anim->hashName = hashName;
    anim->name = hashName;
    anim->name.StripPath();
    anim->name.StripFileExtension();
    anim->refCount = 1;
    
    animHashMap.Set(anim->hashName, anim);

    return anim;
}

void AnimManager::RenameAnim(Anim *anim, const Str &newName) {
    const auto *entry = animHashMap.Get(anim->hashName);
    if (entry) {
        animHashMap.Remove(anim->hashName);

        anim->hashName = newName;
        anim->name = newName;
        anim->name.StripPath();
        anim->name.StripFileExtension();

        animHashMap.Set(newName, anim);
    }
}

void AnimManager::ReleaseAnim(Anim *anim, bool immediateDestroy) {
    if (anim->refCount > 0) {
        anim->refCount--;
    }

    if (immediateDestroy && anim->refCount == 0) {
        DestroyAnim(anim);
    }
}

void AnimManager::DestroyAnim(Anim *anim) {
    if (anim->refCount > 1) {
        BE_LOG(L"AnimManager::DestroyAnim: material '%hs' has %i reference count\n", anim->hashName.c_str(), anim->refCount);
    }

    animHashMap.Remove(anim->hashName);

    delete anim;
}

void AnimManager::DestroyUnusedAnims() {
    Array<Anim *> removeArray;

    for (int i = 0; i < animHashMap.Count(); i++) {
        const auto *entry = animHashMap.GetByIndex(i);
        Anim *anim = entry->second;

        if (anim && anim->refCount == 0) {
            removeArray.Append(anim);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyAnim(removeArray[i]);
    }
}

void AnimManager::PrecacheAnim(const char *name) {
    Anim *anim = GetAnim(name);
    ReleaseAnim(anim);
}

int	AnimManager::JointIndexByName(const char *name) {
    int index;
    int hash = jointNameHash.GenerateHash(name);
    for (index = jointNameHash.First(hash); index != -1; index = jointNameHash.Next(index)) {
        if (jointNameList[index].Cmp(name) == 0) {
            return index;
        }
    }

    index = jointNameList.Append(name);
    jointNameHash.Add(hash, index);
    return index;
}

const char *AnimManager::JointNameByIndex(int index) const {
    return jointNameList[index];
}

Anim *AnimManager::FindAnim(const char *hashName) const {
    const auto *entry = animHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Anim *AnimManager::GetAnim(const char *name) {
    Anim *anim = FindAnim(name);
    if (anim) {
        anim->refCount++;
        return anim;
    }

    anim = AllocAnim(name);
    if (!anim->Load(name)) {
        BE_WARNLOG(L"Couldn't load anim '%hs'\n", name);
        DestroyAnim(anim);
        anim = nullptr;
    }

    return anim;
}

Anim *AnimManager::GetDefaultAnim(const char *name, const Skeleton *skeleton) {	
    Anim *anim = FindAnim(name);
    if (anim) {
        anim->refCount++;
        return anim;
    }

    anim = AllocAnim(name);
    anim->CreateDefaultAnim(skeleton);

    return anim;
}

void AnimManager::ReloadAnims() {
    for (int i = 0; i < animHashMap.Count(); i++) {
        const auto *entry = animHashMap.GetByIndex(i);
        Anim *anim = entry->second;

        if (anim) {
            anim->Reload();
        }
    }
}

//--------------------------------------------------------------------------------------------------

void AnimManager::Cmd_ListAnims(const CmdArgs &args) {
    int num = 0;
    size_t size = 0;

    for (int i = 0; i < animManager.animHashMap.Count(); i++) {
        const auto *entry = animManager.animHashMap.GetByIndex(i);
        Anim *anim = entry->second;

        if (anim) {
            size_t s = anim->Size();
            BE_LOG(L"%2i refs %9hs %.2f secs: %hs\n", 
                anim->refCount, Str::FormatBytes((int)s).c_str(), anim->animLength / 1000.0f, anim->hashName.c_str());

            size += s;
            num++;
        }
    }

    size_t namesize = animManager.jointNameList.Size() + animManager.jointNameHash.Size();
    for (int i = 0; i < animManager.jointNameList.Count(); i++) {
        namesize += animManager.jointNameList[i].Size();
    }

    BE_LOG(L"total %hs used in %i anims\n", Str::FormatBytes((int)size).c_str(), num);
    BE_LOG(L"total %hs used in %i joint names\n", Str::FormatBytes((int)namesize).c_str(), animManager.jointNameList.Count());
}

BE_NAMESPACE_END
