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
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

const SignalDef AssetImporter::SIG_ApplyChanged("AssetImporter::ApplyChanged");

ABSTRACT_DECLARATION("AssetImporter", AssetImporter, Object)
BEGIN_EVENTS(AssetImporter)
END_EVENTS

void AssetImporter::RegisterProperties() {
}

AssetImporter::AssetImporter() {
    asset = nullptr;
}

AssetImporter::~AssetImporter() {
}

Str AssetImporter::ToString() const {
    return asset->ToString();
}

void AssetImporter::RevertChanged() {
    Str metaFilename = Asset::GetMetaFilenameFromAssetPath(asset->GetAssetFilename());

    Json::Value metaDataValue;
    bool validRootNode = false;
    char *text;
    if (fileSystem.LoadFile(metaFilename, false, (void **)&text) > 0) {
        Json::Reader jsonReader;
        validRootNode = jsonReader.parse(text, metaDataValue);
    }

    if (validRootNode) {
        Json::Value importerValue = metaDataValue["importer"];
        Deserialize(importerValue);
    }
}

void AssetImporter::ApplyChanged() {
    Import();

    asset->Reload();

    asset->WriteMetaDataFile();

    EmitSignal(&SIG_ApplyChanged);
}

BE_NAMESPACE_END
