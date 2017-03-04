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
#include "Components/ComTransform.h"
#include "Components/ComTextRenderer.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Text Renderer", ComTextRenderer, ComRenderable)
BEGIN_EVENTS(ComTextRenderer)
END_EVENTS
BEGIN_PROPERTIES(ComTextRenderer)
    PROPERTY_STRING("text", "Text", "", "Hello World", PropertySpec::ReadWrite | PropertySpec::MultiLines),
    PROPERTY_ENUM("textAnchor", "Anchor", "", "Upper Left;Upper Center;Upper Right;Middle Left;Middle Center;Middle Right;Lower Left;Lower Center;Lower Right", "0", PropertySpec::ReadWrite),
    PROPERTY_ENUM("textAlignment", "Alignment", "", "Left;Center;Right", "0", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("lineSpacing", "Line Spacing", "", "1.0", PropertySpec::ReadWrite),
    PROPERTY_OBJECT("font", "Font", "", GuidMapper::defaultFontGuid.ToString(), FontAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_INT("fontSize", "Font Size", "", "14", PropertySpec::ReadWrite),
END_PROPERTIES

void ComTextRenderer::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Text", Str, GetText, SetText, "Hello World", "", PropertySpec::ReadWrite);
}

ComTextRenderer::ComTextRenderer() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComTextRenderer::PropertyChanged);
}

ComTextRenderer::~ComTextRenderer() {
    Purge(false);
}

void ComTextRenderer::Purge(bool chainPurge) {
    if (sceneEntity.font) {
        fontManager.ReleaseFont(sceneEntity.font);
        sceneEntity.font = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComTextRenderer::Init() {
    ComRenderable::Init();

    const Guid fontGuid = props->Get("font").As<Guid>();
    int fontSize = props->Get("fontSize").As<int>();
    ChangeFont(fontGuid, fontSize);

    sceneEntity.layer          = GetEntity()->GetLayer();
    sceneEntity.mesh           = nullptr;
    sceneEntity.text           = Str::ToWStr(props->Get("text").As<Str>());
    sceneEntity.textAnchor     = (SceneEntity::TextAnchor)props->Get("textAnchor").As<int>();
    sceneEntity.textAlignment  = (SceneEntity::TextAlignment)props->Get("textAlignment").As<int>();
    sceneEntity.textScale      = 1.0f;
    sceneEntity.lineSpacing    = props->Get("lineSpacing").As<float>();

    UpdateAABB();
    UpdateVisuals();
}

void ComTextRenderer::SetText(const char *text) {
    sceneEntity.text = Str::ToWStr(text);

    UpdateAABB();
    UpdateVisuals();
}

void ComTextRenderer::UpdateAABB() {
    sceneEntity.aabb = GetGameWorld()->GetRenderWorld()->GetTextMesh().Compute3DTextAABB(sceneEntity.font, sceneEntity.textAnchor, sceneEntity.lineSpacing, sceneEntity.textScale, sceneEntity.text);
}

void ComTextRenderer::ChangeFont(const Guid fontGuid, int fontSize) {
    if (sceneEntity.font) {
        fontManager.ReleaseFont(sceneEntity.font);
    }

    const Str fontPath = resourceGuidMapper.Get(fontGuid);
    sceneEntity.font = fontManager.GetFont(fontPath, fontSize);
}

void ComTextRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "font")) {
        ChangeFont(props->Get("font").As<Guid>(), props->Get("fontSize").As<int>());
        UpdateAABB();
        UpdateVisuals();
        return;
    }

    if (!Str::Cmp(propName, "fontSize")) {
        ChangeFont(props->Get("font").As<Guid>(), props->Get("fontSize").As<int>());
        UpdateAABB();
        UpdateVisuals();
        return;
    }

    if (!Str::Cmp(propName, "text")) {
        SetText(props->Get("text").As<Str>());
        return;
    }

    if (!Str::Cmp(propName, "textAnchor")) {
        sceneEntity.textAnchor = (SceneEntity::TextAnchor)props->Get("textAnchor").As<int>();
        UpdateAABB();
        UpdateVisuals();
        return;
    }

    if (!Str::Cmp(propName, "textAlignment")) {
        sceneEntity.textAlignment = (SceneEntity::TextAlignment)props->Get("textAlignment").As<int>();
        UpdateAABB();
        UpdateVisuals();
        return;
    }

    if (!Str::Cmp(propName, "lineSpacing")) {
        sceneEntity.lineSpacing = props->Get("lineSpacing").As<float>();
        UpdateAABB();
        UpdateVisuals();
        return;
    }    

    ComRenderable::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
