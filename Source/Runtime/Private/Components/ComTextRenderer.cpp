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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Text Renderer", ComTextRenderer, ComRenderable)
BEGIN_EVENTS(ComTextRenderer)
END_EVENTS

void ComTextRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("text", "Text", Str, GetText, SetText, "Hello World", 
        "The text that will be rendered.", PropertyInfo::Flag::Editor | PropertyInfo::Flag::MultiLines);
    REGISTER_MIXED_ACCESSOR_PROPERTY("font", "Font", Guid, GetFontGuid, SetFontGuid, GuidMapper::defaultFontGuid,
        "The TrueType Font to use when rendering the text.", PropertyInfo::Flag::Editor).SetMetaObject(&FontResource::metaObject);
    REGISTER_ACCESSOR_PROPERTY("fontSize", "Font Size", int, GetFontSize, SetFontSize, 14,
        "The size of the font.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("drawMode", "Draw Mode", RenderObject::TextDrawMode::Enum, GetDrawMode, SetDrawMode, RenderObject::TextDrawMode::Normal,
        "The font drawing mode.", PropertyInfo::Flag::Editor).SetEnumString("Normal;Drop Shadows;Add Outlines");
    REGISTER_MIXED_ACCESSOR_PROPERTY("textSecondaryColor", "Secondary Color", Color3, GetSecondaryColor, SetSecondaryColor, Color3::black,
        "The color for drawing shadows or outlines.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("textSecondaryAlpha", "Secondary Alpha", float, GetSecondaryAlpha, SetSecondaryAlpha, 1.f,
        "The alpha for drawing shadows or outlines.", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("lineSpacing", "Line Spacing", float, GetLineSpacing, SetLineSpacing, 1.f,
        "How much space will be in-between lines of text.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("textAnchor", "Anchor", RenderObject::TextAnchor::Enum, GetAnchor, SetAnchor, RenderObject::TextAnchor::UpperLeft,
        "Which point of the text shares the position of the Transform.", PropertyInfo::Flag::Editor).SetEnumString("Upper Left;Upper Center;Upper Right;Middle Left;Middle Center;Middle Right;Lower Left;Lower Center;Lower Right");
    REGISTER_ACCESSOR_PROPERTY("textAlignment", "Alignment", RenderObject::TextHorzAlignment::Enum, GetAlignment, SetAlignment, RenderObject::TextHorzAlignment::Center,
        "How lines of text are aligned (Left, Right, Center).", PropertyInfo::Flag::Editor).SetEnumString("Left;Center;Right");
}

ComTextRenderer::ComTextRenderer() {
}

ComTextRenderer::~ComTextRenderer() {
    Purge(false);
}

void ComTextRenderer::Purge(bool chainPurge) {
    if (renderObjectDef.font) {
        fontManager.ReleaseFont(renderObjectDef.font);
        renderObjectDef.font = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComTextRenderer::Init() {
    ComRenderable::Init();

    renderObjectDef.textScale = CmToUnit(1.0f);

    // Mark as initialized
    SetInitialized(true);

    ChangeFont(fontGuid, fontSize);

    UpdateAABB();

    UpdateVisuals();
}

Str ComTextRenderer::GetText() const {
    return renderObjectDef.text;
}

void ComTextRenderer::SetText(const Str &text) {
    renderObjectDef.text = text;

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

RenderObject::TextAnchor::Enum ComTextRenderer::GetAnchor() const {
    return renderObjectDef.textAnchor;
}

void ComTextRenderer::SetAnchor(RenderObject::TextAnchor::Enum anchor) {
    renderObjectDef.textAnchor = anchor;

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

RenderObject::TextHorzAlignment::Enum ComTextRenderer::GetAlignment() const {
    return renderObjectDef.textHorzAlignment;
}

void ComTextRenderer::SetAlignment(RenderObject::TextHorzAlignment::Enum alignment) {
    renderObjectDef.textHorzAlignment = alignment;
    
    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

RenderObject::TextDrawMode::Enum ComTextRenderer::GetDrawMode() const {
    return renderObjectDef.textDrawMode;
}

void ComTextRenderer::SetDrawMode(RenderObject::TextDrawMode::Enum drawMode) {
    renderObjectDef.textDrawMode = drawMode;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

Color3 ComTextRenderer::GetSecondaryColor() const {
    return renderObjectDef.textFxColor.ToColor3();
}

void ComTextRenderer::SetSecondaryColor(const Color3 &color) {
    renderObjectDef.textFxColor.r = color.r;
    renderObjectDef.textFxColor.g = color.g;
    renderObjectDef.textFxColor.b = color.b;

    UpdateVisuals();
}

float ComTextRenderer::GetSecondaryAlpha() const {
    return renderObjectDef.textFxColor.a;
}

void ComTextRenderer::SetSecondaryAlpha(float alpha) {
    renderObjectDef.textFxColor.a = alpha;

    UpdateVisuals();
}

float ComTextRenderer::GetLineSpacing() const {
    return renderObjectDef.lineSpacing;
}

void ComTextRenderer::SetLineSpacing(float lineSpacing) {
    renderObjectDef.lineSpacing = lineSpacing;

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

Guid ComTextRenderer::GetFontGuid() const {
    return fontGuid;
}

void ComTextRenderer::SetFontGuid(const Guid &fontGuid) {
    this->fontGuid = fontGuid;

    if (IsInitialized()) {
        ChangeFont(fontGuid, fontSize);

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
    renderObjectDef.aabb = GetGameWorld()->GetRenderWorld()->GetTextMesh().Compute3DTextAABB(renderObjectDef.font, 
        renderObjectDef.textAnchor, renderObjectDef.lineSpacing, renderObjectDef.textScale, renderObjectDef.text);
}

void ComTextRenderer::ChangeFont(const Guid &fontGuid, int fontSize) {
    if (renderObjectDef.font) {
        fontManager.ReleaseFont(renderObjectDef.font);
    }

    const Str fontPath = resourceGuidMapper.Get(fontGuid);
    renderObjectDef.font = fontManager.GetFont(fontPath, fontSize);
}

BE_NAMESPACE_END
