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
#include "Asset/GuidMapper.h"
#include "RenderInternal.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

const int Skin::GetNumMeshAssociations() const {
    return associatedMeshes.Count();
}

const char *Skin::GetAssociatedMesh(int index) const {
    assert(index > 0 && index < associatedMeshes.Count());
    return associatedMeshes[index].c_str();
}

const Material *Skin::RemapMaterialBySkin(const Material *material) const {
    int numMappings = mappingList.Count();

    const Material *replaced = material;

    for (int i = 0; i < numMappings; i++) {
        const SkinMapping *mapping = &mappingList[i];

        if (!mapping->from && replaced == material) {
            replaced = mapping->to;
        }

        if (mapping->from == material) {
            return mapping->to;
        }
    }

    return replaced;
}

bool Skin::Create(const char *text) {
    SkinMapping skinMapping;	
    Str token;
    Str token2;

    Purge();

    Lexer lexer; 
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName.c_str());

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        BE_WARNLOG(L"expecting '{', found '%hs' instead in skin '%hs'\n", token.c_str(), hashName.c_str());
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (!token[0]) {
            BE_WARNLOG(L"no concluding '}' in skin %hs\n", hashName.c_str());
            return false;
        }
        else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("mesh")) {
            if (lexer.ReadToken(&token, false)) {
                associatedMeshes.AddUnique(token);
            } else {
                BE_WARNLOG(L"missing parameter mesh keyword in skin '%hs'\n", hashName.c_str());
            }
        } else {
            lexer.ReadToken(&token2, false);

            if (!token.Cmp("_all")) {
                skinMapping.from = nullptr;
            } else {
                const Guid fromGuid = Guid::FromString(token);
                const Str fromPath = resourceGuidMapper.Get(fromGuid);
                skinMapping.from = materialManager.GetMaterial(fromPath);
            }

            const Guid toGuid = Guid::FromString(token2);
            const Str toPath = resourceGuidMapper.Get(toGuid);
            skinMapping.to = materialManager.GetMaterial(toPath);
            
            mappingList.Append(skinMapping);
        }
    }

    return true;
}

void Skin::Purge() {
    mappingList.Clear();
    associatedMeshes.Clear();
}

bool Skin::Load(const char *filename) {
    char *data;

    size_t fileSize = fileSystem.LoadFile(filename, false, (void **)&data);
    if (!data) {
        return false;
    }

    bool ret = Create(data);

    fileSystem.FreeFile(data);

    return ret;
}

bool Skin::Reload() {
    Str _hashName = hashName;
    return Load(_hashName);
}

void Skin::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Skin::Write: file open error\n");
        return;
    }
    /*
    for (int i = 0; i < mappingList.Count(); i++) {
        const SkinMapping *mapping = &mappingList[i];
    
        const Asset *fromAsset = assetDatabase->FindAssetByAssetPath(mapping->from->GetName());
        const SkinAsset *fromSkinAsset = fromAsset ? fromAsset->Cast<SkinAsset>() : nullptr;
        const Guid fromAssetGuid = fromSkinAsset ? fromSkinAsset->GetGuid() : 0;

        const Asset *toAsset = assetDatabase->FindAssetByAssetPath(mapping->to->GetName());
        const SkinAsset *toSkinAsset = toAsset ? toAsset->Cast<SkinAsset>() : nullptr;
        const Guid toAssetGuid = toSkinAsset ? toSkinAsset->GetGuid() : 0;

        fp->Printf("\"%s\" \"%s\"\n", fromAssetGuid.ToString(), toAssetGuid.ToString());
    }*/

    fileSystem.CloseFile(fp);
}

BE_NAMESPACE_END
