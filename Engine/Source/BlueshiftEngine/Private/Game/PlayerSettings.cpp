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
#include "File/FileSystem.h"
#include "Game/PlayerSettings.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Player Settings", PlayerSettings, Object)
BEGIN_EVENTS(PlayerSettings)
END_EVENTS

void PlayerSettings::RegisterProperties() {
    REGISTER_PROPERTY("companyName", "Company Name", Str, companyName, "DefaultCompany", "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("productName", "Product Name", Str, productName, "MyApp", "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("appScript", "App Script", Guid, appScriptGuid, Guid::zero, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&ScriptAsset::metaObject);
}

PlayerSettings::PlayerSettings() {
}

PlayerSettings *PlayerSettings::Load(const char *filename) {
    Json::Value jsonNode;
    Json::Reader jsonReader;
    bool failedToParse = false;

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (text) {
        if (!jsonReader.parse(text, jsonNode)) {
            BE_WARNLOG(L"Failed to parse JSON text '%hs'\n", filename);
            failedToParse = true;
        }

        fileSystem.FreeFile(text);
    } else {
        failedToParse = true;
    }

    if (failedToParse) {
        jsonNode["classname"] = PlayerSettings::metaObject.ClassName();
        jsonNode["guid"] = Guid::CreateGuid().ToString();
    }

    const char *classname = jsonNode["classname"].asCString();

    if (Str::Cmp(classname, PlayerSettings::metaObject.ClassName())) {
        BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
        return nullptr;
    }

    const Guid guid = Guid::FromString(jsonNode["guid"].asCString());

    PlayerSettings *playerSettings = (PlayerSettings *)PlayerSettings::CreateInstance(guid);
    playerSettings->Deserialize(jsonNode);

    return playerSettings;
}

BE_NAMESPACE_END
