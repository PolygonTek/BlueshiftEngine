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
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Asset", Asset, Object)
BEGIN_EVENTS(Asset)
END_EVENTS

const SignalDef Asset::SIG_Reloaded("Asset::Reloaded");
const SignalDef Asset::SIG_Modified("Asset::Modified", "i");

void Asset::RegisterProperties() {
    REGISTER_PROPERTY("timeStamp", "Time Stamp", Str, timeStamp, "0", "", 0);
}

Asset::Asset() {
    node.SetOwner(this);
    assetImporter = nullptr;
    isStoredInDisk = false;
    isRedundantAsset = false;
}

Asset::~Asset() {
    if (assetImporter) {
        AssetImporter::DestroyInstanceImmediate(assetImporter);
    }
}

const Str Asset::NormalizeAssetPath(const Str &assetPath) {
    Str normalizedAssetPath = assetPath;

    Str::ConvertPathSeperator(normalizedAssetPath, PATHSEPERATOR_CHAR);

    return normalizedAssetPath;
}

const Str Asset::GetMetaFilenameFromAssetPath(const char *assetPath) {
    Str metaFilename = Asset::NormalizeAssetPath(assetPath);
    metaFilename += ".meta";

    return metaFilename;
}

const Str Asset::GetCacheDirectory(const char *baseLibraryDir) const {
    Str cacheDir = baseLibraryDir;
    cacheDir.AppendPath("metadata");
    cacheDir.AppendPath(va("%s", GetGuid().ToString(Guid::DigitsWithHyphens)));

    Str::ConvertPathSeperator(cacheDir, PATHSEPERATOR_CHAR);
    
    return cacheDir;
}

const Str Asset::GetAssetFilename() const {
    Str assetPath;
    const Asset *asset = this;
    
    while (asset) {
        Str tempPath = asset->name;
        tempPath.AppendPath(assetPath, PATHSEPERATOR_CHAR);
        assetPath = tempPath;

        asset = asset->node.GetParent();
    }

    return assetPath;
}

const Str Asset::GetResourceFilename() const {
    Str name;

    if (assetImporter) {
        name = Asset::NormalizeAssetPath(assetImporter->GetCacheFilename());
    } else {
        name = Asset::NormalizeAssetPath(GetAssetFilename());
    }

    return name;
}

void Asset::Rename(const Str &newName) {
    name = newName;
    name.StripPath();
}

void Asset::GetChildren(Array<Asset *> &children) const {
    for (Asset *child = node.GetChild(); child; child = child->node.GetNextSibling()) {
        children.Append(child);
        child->GetChildren(children);
    }
}

void Asset::WriteMetaDataFile() const {
    Str metaFilename = Asset::GetMetaFilenameFromAssetPath(GetAssetFilename());

    Json::Value metaDataValue;
    Serialize(metaDataValue);

    if (assetImporter) {
        Json::Value importerValue;
        assetImporter->Serialize(importerValue);

        metaDataValue["importer"] = importerValue;
    }
 
    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(metaDataValue).c_str();

    fileSystem.WriteFile(metaFilename, jsonText.c_str(), jsonText.Length());
}

void Asset::SetModified(bool modified) {
    EmitSignal(&SIG_Modified, modified ? 1 : 0);
}

BE_NAMESPACE_END
