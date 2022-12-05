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
#include "Components/Physics/ComCloth.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cloth", ComCloth, Component)
BEGIN_EVENTS(ComCloth)
END_EVENTS

void ComCloth::RegisterProperties() {
    REGISTER_PROPERTY("stretchingStiffness", "Stretching Stiffness", float, stretchingStiffness, 1.0f,
        "", PropertyInfo::Flag::Editor);
}

ComCloth::ComCloth() {
    updatable = true;
}

ComCloth::~ComCloth() {
    Purge(false);
}

void ComCloth::Purge(bool chainPurge) {
    if (chainPurge) {
        Component::Purge();
    }
}

void ComCloth::Init() {
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

#if WITH_EDITOR
void ComCloth::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
}
#endif

BE_NAMESPACE_END
