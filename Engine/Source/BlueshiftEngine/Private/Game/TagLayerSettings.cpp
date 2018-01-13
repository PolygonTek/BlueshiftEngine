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
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Tag & Layer Settings", TagLayerSettings, Object)
BEGIN_EVENTS(TagLayerSettings)
END_EVENTS

void TagLayerSettings::RegisterProperties() {
    REGISTER_ARRAY_PROPERTY("tag", "Tag", Str, tags, "Untagged", "", PropertyInfo::EditorFlag);
    REGISTER_ARRAY_PROPERTY("layer", "Layer", Str, layers, "Default", "", PropertyInfo::EditorFlag);
}

TagLayerSettings::TagLayerSettings() {
}

int32_t TagLayerSettings::FindTag(const char *tagName) const {
    for (int i = 0; i < tags.Count(); i++) {
        if (tags[i] == tagName) {
            return i;
        }
    }
    return -1;
}

int32_t TagLayerSettings::FindLayer(const char *layerName) const {
    for (int i = 0; i < layers.Count(); i++) {
        if (layers[i] == layerName) {
            return i;
        }
    }
    return -1;
}

TagLayerSettings *TagLayerSettings::Load(const char *filename) {
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
        jsonNode["classname"] = TagLayerSettings::metaObject.ClassName();

        // default tags
        jsonNode["tag"][0] = "Untagged";
        jsonNode["tag"][1] = "MainCamera";
        jsonNode["tag"][2] = "Player";

        // default layers
        jsonNode["layer"][0] = "Default";
        jsonNode["layer"][1] = "UI";
        jsonNode["layer"][2] = "Editor";
    }

    const char *classname = jsonNode["classname"].asCString();

    if (Str::Cmp(classname, TagLayerSettings::metaObject.ClassName())) {
        BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
        return nullptr;
    }

    const Guid guid = Guid::FromString(jsonNode["guid"].asCString());

    TagLayerSettings *tagLayerSettings = (TagLayerSettings *)TagLayerSettings::CreateInstance(guid);
    tagLayerSettings->Deserialize(jsonNode);

    return tagLayerSettings;
}

BE_NAMESPACE_END
