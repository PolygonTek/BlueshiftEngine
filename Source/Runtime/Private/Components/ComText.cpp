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
    REGISTER_ACCESSOR_PROPERTY("vertAlignment", "Vertical Alignment", RenderObject::TextVertAlignment::Enum, GetVertAlignment, SetVertAlignment, 0, 
        "(Top, Middle, Bottom)", PropertyInfo::Flag::Editor).SetEnumString("Upper Left;Upper Center;Upper Right;Middle Left;Middle Center;Middle Right;Lower Left;Lower Center;Lower Right");
    REGISTER_ACCESSOR_PROPERTY("horzAlignment", "Horizontal Alignment", RenderObject::TextHorzAlignment::Enum, GetHorzAlignment, SetHorzAlignment, 0, 
        "How lines of text are aligned (Left, Center, Right).", PropertyInfo::Flag::Editor).SetEnumString("Left;Center;Right");
    REGISTER_ACCESSOR_PROPERTY("lineSpacing", "Line Spacing", float, GetLineSpacing, SetLineSpacing, 1.f, 
        "How much space will be in-between lines of text.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("fontSize", "Font Size", int, GetFontSize, SetFontSize, 14, 
        "The size of the font.", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("font", "Font", Guid, GetFontGuid, SetFontGuid, GuidMapper::defaultFontGuid, 
        "The TrueType Font to use when rendering the text.", PropertyInfo::Flag::Editor).SetMetaObject(&FontResource::metaObject);
}

ComText::ComText() {
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

    renderObjectDef.aabb = rectTransform->GetAABB();

    rectTransform->Connect(&ComRectTransform::SIG_RectTransformUpdated, this, (SignalCallback)&ComText::RectTransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized
    SetInitialized(true);

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

RenderObject::TextVertAlignment::Enum ComText::GetVertAlignment() const {
    return renderObjectDef.textVertAlignment;
}

void ComText::SetVertAlignment(RenderObject::TextVertAlignment::Enum vertAlignment) {
    renderObjectDef.textVertAlignment = vertAlignment;

    if (IsInitialized()) {
        UpdateVisuals();
    }
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
    if (renderObjectDef.font) {
        const Str fontPath = renderObjectDef.font->GetHashName();
        return resourceGuidMapper.Get(fontPath);
    }
    return Guid();
}

void ComText::SetFontGuid(const Guid &fontGuid) {
    ChangeFont(fontGuid, fontSize);
    
    if (IsInitialized()) {
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
