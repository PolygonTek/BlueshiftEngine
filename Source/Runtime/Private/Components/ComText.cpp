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
#include "Components/ComRectTransform.h"
#include "Components/ComText.h"
#include "Game/GameWorld.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Text", ComText, ComRenderable)
BEGIN_EVENTS(ComText)
END_EVENTS

void ComText::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("text", "Text", Str, GetText, SetText, "Hello World", 
        "The text that will be rendered.", PropertyInfo::Flag::Editor | PropertyInfo::Flag::MultiLines);
    REGISTER_MIXED_ACCESSOR_PROPERTY("font", "Font", Guid, GetFontGuid, SetFontGuid, GuidMapper::defaultFontGuid,
        "The TrueType Font to use when rendering the text.", PropertyInfo::Flag::Editor).SetMetaObject(&FontResource::metaObject);
    REGISTER_ACCESSOR_PROPERTY("fontSize", "Font Size", int, GetFontSize, SetFontSize, 14,
        "The size of the font.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("renderMode", "Render Mode", Font::RenderMode::Enum, GetRenderMode, SetRenderMode, Font::RenderMode::Normal,
        "The font rendering mode.", PropertyInfo::Flag::Editor).SetEnumString("Normal;Drop Shadows;Add Outlines");
    REGISTER_ACCESSOR_PROPERTY("lineSpacing", "Line Spacing", float, GetLineSpacing, SetLineSpacing, 1.f,
        "How much space will be in-between lines of text.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("horzAlignment", "Horizontal Alignment", RenderObject::TextHorzAlignment::Enum, GetHorzAlignment, SetHorzAlignment, RenderObject::TextHorzAlignment::Center,
        "How lines of text are aligned (Left, Center, Right).", PropertyInfo::Flag::Editor).SetEnumString("Left;Center;Right");
    REGISTER_ACCESSOR_PROPERTY("vertAlignment", "Vertical Alignment", RenderObject::TextVertAlignment::Enum, GetVertAlignment, SetVertAlignment, RenderObject::TextVertAlignment::Middle,
        "(Top, Middle, Bottom)", PropertyInfo::Flag::Editor).SetEnumString("Top;Middle;Bottom");
    REGISTER_ACCESSOR_PROPERTY("horzOverflow", "Horizontal Overflow", RenderObject::TextHorzOverflow::Enum, GetHorzOverflow, SetHorzOverflow, RenderObject::TextHorzOverflow::Wrap,
        "How lines of text are aligned (Left, Center, Right).", PropertyInfo::Flag::Editor).SetEnumString("Wrap;Overflow");
    REGISTER_ACCESSOR_PROPERTY("vertOverflow", "Vertical Overflow", RenderObject::TextVertOverflow::Enum, GetVertOverflow, SetVertOverflow, RenderObject::TextVertOverflow::Truncate,
        "(Top, Middle, Bottom)", PropertyInfo::Flag::Editor).SetEnumString("Truncate;Overflow");
}

ComText::ComText() {
    fontGuid = Guid();
    fontSize = 0;
}

ComText::~ComText() {
    Purge(false);
}

void ComText::Purge(bool chainPurge) {
    if (renderObjectDef.font) {
        fontManager.ReleaseFont(renderObjectDef.font);
        renderObjectDef.font = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComText::Init() {
    ComRenderable::Init();

    // Don't account maxVisDist when rendering this render object.
    renderObjectDef.flags |= RenderObject::Flag::NoVisDist;

    renderObjectDef.textScale = MeterToUnit(1.0f);

    ComRectTransform *rectTransform = GetEntity()->GetRectTransform();

    renderObjectDef.textRect = rectTransform->GetLocalRect();
    renderObjectDef.aabb = rectTransform->GetAABB();

    rectTransform->Connect(&ComRectTransform::SIG_RectTransformUpdated, this, (SignalCallback)&ComText::RectTransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized
    SetInitialized(true);

    ChangeFont(fontGuid, fontSize);

    UpdateVisuals();
}

Str ComText::GetText() const {
    return renderObjectDef.text;
}

void ComText::SetText(const Str &text) {
    renderObjectDef.text = text;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

const char *ComText::GetTextCString() const {
    static Str text;
    text = GetText();
    return text.c_str();
}

void ComText::SetTextCString(const char *text) {
    SetText(Str(text));
}

RenderObject::TextHorzAlignment::Enum ComText::GetHorzAlignment() const {
    return renderObjectDef.textHorzAlignment;
}

void ComText::SetHorzAlignment(RenderObject::TextHorzAlignment::Enum horzAlignment) {
    renderObjectDef.textHorzAlignment = horzAlignment;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

RenderObject::TextVertAlignment::Enum ComText::GetVertAlignment() const {
    return renderObjectDef.textVertAlignment;
}

void ComText::SetVertAlignment(RenderObject::TextVertAlignment::Enum vertAlignment) {
    renderObjectDef.textVertAlignment = vertAlignment;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

RenderObject::TextHorzOverflow::Enum ComText::GetHorzOverflow() const {
    return renderObjectDef.textHorzOverflow;
}

void ComText::SetHorzOverflow(RenderObject::TextHorzOverflow::Enum horzOverflow) {
    renderObjectDef.textHorzOverflow = horzOverflow;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

RenderObject::TextVertOverflow::Enum ComText::GetVertOverflow() const {
    return renderObjectDef.textVertOverflow;
}

void ComText::SetVertOverflow(RenderObject::TextVertOverflow::Enum vertOverflow) {
    renderObjectDef.textVertOverflow = vertOverflow;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

Font::RenderMode::Enum ComText::GetRenderMode() const {
    return renderObjectDef.fontRenderMode;
}

void ComText::SetRenderMode(Font::RenderMode::Enum renderMode) {
    renderObjectDef.fontRenderMode = renderMode;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

float ComText::GetLineSpacing() const {
    return renderObjectDef.lineSpacing;
}

void ComText::SetLineSpacing(float lineSpacing) {
    renderObjectDef.lineSpacing = lineSpacing;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

Guid ComText::GetFontGuid() const {
    return fontGuid;
}

void ComText::SetFontGuid(const Guid &fontGuid) {
    this->fontGuid = fontGuid;

    if (IsInitialized()) {
        ChangeFont(fontGuid, fontSize);

        UpdateVisuals();
    }
}

int ComText::GetFontSize() const {
    return fontSize;
}

void ComText::SetFontSize(int fontSize) {
    this->fontSize = fontSize;

    if (IsInitialized()) {
        ChangeFont(GetFontGuid(), fontSize);

        UpdateVisuals();
    }
}

void ComText::RectTransformUpdated(const ComRectTransform *rectTransform) {
    renderObjectDef.textRect = rectTransform->GetLocalRect();
    renderObjectDef.aabb = rectTransform->GetAABB();

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComText::ChangeFont(const Guid &fontGuid, int fontSize) {
    if (renderObjectDef.font) {
        fontManager.ReleaseFont(renderObjectDef.font);
    }

    const Str fontPath = resourceGuidMapper.Get(fontGuid);
    renderObjectDef.font = fontManager.GetFont(fontPath, fontSize);
}

BE_NAMESPACE_END
