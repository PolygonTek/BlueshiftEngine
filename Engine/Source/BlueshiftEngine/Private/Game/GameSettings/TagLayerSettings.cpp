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
BEGIN_PROPERTIES(TagLayerSettings)
    PROPERTY_STRING("tag", "Tag", "tag", "Untagged", PropertyInfo::ReadWrite | PropertyInfo::IsArray),
    PROPERTY_STRING("layer", "Layer", "layer", "Default", PropertyInfo::ReadWrite | PropertyInfo::IsArray),
END_PROPERTIES

#ifdef NEW_PROPERTY_SYSTEM
void TagLayerSettings::RegisterProperties() {
}
#endif

TagLayerSettings::TagLayerSettings() {
}

void TagLayerSettings::Init() {
    GameSettings::Init();
}

int32_t TagLayerSettings::FindTag(const char *tagName) const {
    int numTags = props->NumElements("tag");
    for (int i = 0; i < numTags; i++) {
        Str name = va("tag[%i]", i);

        if (props->Get(name).As<Str>() == tagName) {
            return i;
        }
    }
    return -1;
}

int32_t TagLayerSettings::FindLayer(const char *layerName) const {
    int numLayers = props->NumElements("layer");
    for (int i = 0; i < numLayers; i++) {
        Str name = va("layer[%i]", i);

        if (props->Get(name).As<Str>() == layerName) {
            return i;
        }
    }
    return -1;
}

void TagLayerSettings::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    GameSettings::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END