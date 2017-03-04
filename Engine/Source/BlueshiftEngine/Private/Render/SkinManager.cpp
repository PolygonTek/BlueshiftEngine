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
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

Skin *          SkinManager::defaultSkin;

SkinManager     skinManager;

void SkinManager::Init() {
    cmdSystem.AddCommand(L"listSkins", Cmd_ListSkins);

    skinHashMap.Init(1024, MAX_SKINS, 1024);

    defaultSkin = AllocSkin("_defaultSkin");
    defaultSkin->Create("{ _all _default\n }");
    defaultSkin->permanence = true;
}

void SkinManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listSkins");

    skinHashMap.DeleteContents(true);
}

Skin *SkinManager::AllocSkin(const char *hashName) {
    if (skinHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs skin already allocated", hashName);
    }
    
    Skin *skin = new Skin;
    skin->refCount = 1;
    skin->hashName = hashName;
    skinHashMap.Set(skin->hashName, skin);
            
    return skin;
}

void SkinManager::DestroySkin(Skin *skin) {
    if (skin->refCount > 1) {
        BE_LOG(L"SkinManager::DestroySkin: skin '%hs' has %i reference count\n", skin->hashName.c_str(), skin->refCount);
    }

    skinHashMap.Remove(skin->hashName);
    delete skin;
}

void SkinManager::DestroyUnusedSkins() {
    Array<Skin *> removeArray;

    for (int i = 0; i < skinHashMap.Count(); i++) {
        const auto *entry = skinHashMap.GetByIndex(i);
        Skin *skin = entry->second;

        if (skin && !skin->permanence && skin->refCount == 0) {
            removeArray.Append(skin);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroySkin(removeArray[i]);
    }
}

Skin *SkinManager::FindSkin(const char *name) const {
    const auto *entry = skinHashMap.Get(Str(name));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Skin *SkinManager::GetSkin(const char *name) {
    Skin *skin = FindSkin(name);
    if (skin) {
        skin->refCount++;
        return skin;
    }

    skin = AllocSkin(name);
    if (!skin->Load(name)) {
        DestroySkin(skin);
        return defaultSkin;
    }

    return skin;
}

void SkinManager::ReleaseSkin(Skin *skin, bool immediateDestroy) {
    if (skin->permanence) {
        return;
    }

    if (skin->refCount > 0) {
        skin->refCount--;
    }

    if (immediateDestroy && skin->refCount == 0) {
        DestroySkin(skin);
    }
}

void SkinManager::Cmd_ListSkins(const CmdArgs &args) {
    int count = 0;

    for (int i = 0; i < skinManager.skinHashMap.Count(); i++) {
        const auto *entry = skinManager.skinHashMap.GetByIndex(i);
        Skin *skin = entry->second;

        BE_LOG(L"%4d:%hs\n", i, skin->hashName.c_str());

        for (int j = 0; j < skin->mappingList.Count(); j++) {
            const Skin::SkinMapping *mapping = &skin->mappingList[j];
            BE_LOG(L"    %hs -> %hs\n", mapping->from, mapping->to);
        }

        count++;
    }

    BE_LOG(L"%i total skins\n", count);
}

BE_NAMESPACE_END
