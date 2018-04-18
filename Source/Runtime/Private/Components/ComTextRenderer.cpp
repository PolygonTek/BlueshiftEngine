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
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Text Renderer", ComTextRenderer, ComRenderable)
BEGIN_EVENTS(ComTextRenderer)
END_EVENTS

void ComTextRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("text", "Text", Str, GetText, SetText, "Hello World", 
        "", PropertyInfo::EditorFlag | PropertyInfo::MultiLinesFlag);
    REGISTER_ACCESSOR_PROPERTY("textAnchor", "Anchor", SceneObject::TextAnchor, GetAnchor, SetAnchor, 0, 
        "", PropertyInfo::EditorFlag).SetEnumString("Upper Left;Upper Center;Upper Right;Middle Left;Middle Center;Middle Right;Lower Left;Lower Center;Lower Right");
    REGISTER_ACCESSOR_PROPERTY("textAlignment", "Alignment", SceneObject::TextAlignment, GetAlignment, SetAlignment, 0, 
        "", PropertyInfo::EditorFlag).SetEnumString("Left; Center; Right");
    REGISTER_ACCESSOR_PROPERTY("lineSpacing", "Line Spacing", float, GetLineSpacing, SetLineSpacing, 1.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("fontSize", "Font Size", int, GetFontSize, SetFontSize, 14, 
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("font", "Font", Guid, GetFontGuid, SetFontGuid, GuidMapper::defaultFontGuid, 
        "", PropertyInfo::EditorFlag).SetMetaObject(&FontAsset::metaObject);
}

ComTextRenderer::ComTextRenderer() {
}

ComTextRenderer::~ComTextRenderer() {
    Purge(false);
}

void ComTextRenderer::Purge(bool chainPurge) {
    if (sceneObjectParms.font) {
        fontManager.ReleaseFont(sceneObjectParms.font);
        sceneObjectParms.font = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComTextRenderer::Init() {
    ComRenderable::Init();

    sceneObjectParms.textScale = 1.0f;

    UpdateAABB();

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

Str ComTextRenderer::GetText() const {
    return Str(sceneObjectParms.text.c_str());
}

void ComTextRenderer::SetText(const Str &text) {
    sceneObjectParms.text = Str::ToWStr(text);

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

const char *ComTextRenderer::GetTextCString() const {
    static Str text;
    text = GetText();
    return text.c_str();
}

void ComTextRenderer::SetTextCString(const char *text) {
    SetText(Str(text));
}

SceneObject::TextAnchor ComTextRenderer::GetAnchor() const {
    return sceneObjectParms.textAnchor;
}

void ComTextRenderer::SetAnchor(SceneObject::TextAnchor anchor) {
    sceneObjectParms.textAnchor = anchor;

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

SceneObject::TextAlignment ComTextRenderer::GetAlignment() const {
    return sceneObjectParms.textAlignment;
}

void ComTextRenderer::SetAlignment(SceneObject::TextAlignment alignment) {
    sceneObjectParms.textAlignment = alignment;
    
    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

float ComTextRenderer::GetLineSpacing() const {
    return sceneObjectParms.lineSpacing;
}

void ComTextRenderer::SetLineSpacing(float lineSpacing) {
    sceneObjectParms.lineSpacing = lineSpacing;

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

Guid ComTextRenderer::GetFontGuid() const {
    if (sceneObjectParms.font) {
        const Str fontPath = sceneObjectParms.font->GetHashName();
        return resourceGuidMapper.Get(fontPath);
    }
    return Guid();
}

void ComTextRenderer::SetFontGuid(const Guid &fontGuid) {
    ChangeFont(fontGuid, fontSize);
    
    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

int ComTextRenderer::GetFontSize() const {
    return fontSize;
}

void ComTextRenderer::SetFontSize(int fontSize) {
    this->fontSize = fontSize;

    if (IsInitialized()) {
        ChangeFont(GetFontGuid(), fontSize);

        UpdateAABB();
        UpdateVisuals();
    }
}

void ComTextRenderer::UpdateAABB() {
    sceneObjectParms.aabb = GetGameWorld()->GetRenderWorld()->GetTextMesh().Compute3DTextAABB(sceneObjectParms.font, sceneObjectParms.textAnchor, sceneObjectParms.lineSpacing, sceneObjectParms.textScale, sceneObjectParms.text);
}

void ComTextRenderer::ChangeFont(const Guid &fontGuid, int fontSize) {
    if (sceneObjectParms.font) {
        fontManager.ReleaseFont(sceneObjectParms.font);
    }

    const Str fontPath = resourceGuidMapper.Get(fontGuid);
    sceneObjectParms.font = fontManager.GetFont(fontPath, fontSize);
}

BE_NAMESPACE_END
