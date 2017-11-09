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
#include "Game/GameSettings/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Tag & Layer Settings", TagLayerSettings, GameSettings)
BEGIN_EVENTS(TagLayerSettings)
END_EVENTS

void TagLayerSettings::RegisterProperties() {
    REGISTER_LIST_PROPERTY("tag", "Tag", Str, tags, "Untagged", "", PropertyInfo::Editor);
    REGISTER_LIST_PROPERTY("layer", "Layer", Str, layers, "Default", "", PropertyInfo::Editor);
}

TagLayerSettings::TagLayerSettings() {
}

void TagLayerSettings::Init() {
    GameSettings::Init();
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

BE_NAMESPACE_END