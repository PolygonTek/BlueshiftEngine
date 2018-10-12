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
        "The text that will be rendered.", PropertyInfo::EditorFlag | PropertyInfo::MultiLinesFlag);
    REGISTER_ACCESSOR_PROPERTY("textAnchor", "Anchor", RenderObject::TextAnchor, GetAnchor, SetAnchor, 0, 
        "Which point of the text shares the position of the Transform.", PropertyInfo::EditorFlag).SetEnumString("Upper Left;Upper Center;Upper Right;Middle Left;Middle Center;Middle Right;Lower Left;Lower Center;Lower Right");
    REGISTER_ACCESSOR_PROPERTY("textAlignment", "Alignment", RenderObject::TextAlignment, GetAlignment, SetAlignment, 0, 
        "How lines of text are aligned (Left, Right, Center).", PropertyInfo::EditorFlag).SetEnumString("Left;Center;Right");
    REGISTER_ACCESSOR_PROPERTY("lineSpacing", "Line Spacing", float, GetLineSpacing, SetLineSpacing, 1.f, 
        "How much space will be in-between lines of text.", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("fontSize", "Font Size", int, GetFontSize, SetFontSize, 14, 
        "The size of the font.", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("font", "Font", Guid, GetFontGuid, SetFontGuid, GuidMapper::defaultFontGuid, 
        "The TrueType Font to use when rendering the text.", PropertyInfo::EditorFlag).SetMetaObject(&FontAsset::metaObject);
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

    renderObjectDef.textScale = CentiToUnit(1.0f);

    UpdateAABB();

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

Str ComTextRenderer::GetText() const {
    return Str(renderObjectDef.text.c_str());
}

void ComTextRenderer::SetText(const Str &text) {
    renderObjectDef.text = Str::ToWStr(text);

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

RenderObject::TextAnchor ComTextRenderer::GetAnchor() const {
    return renderObjectDef.textAnchor;
}

void ComTextRenderer::SetAnchor(RenderObject::TextAnchor anchor) {
    renderObjectDef.textAnchor = anchor;

    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
}

RenderObject::TextAlignment ComTextRenderer::GetAlignment() const {
    return renderObjectDef.textAlignment;
}

void ComTextRenderer::SetAlignment(RenderObject::TextAlignment alignment) {
    renderObjectDef.textAlignment = alignment;
    
    if (IsInitialized()) {
        UpdateAABB();
        UpdateVisuals();
    }
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
    if (renderObjectDef.font) {
        const Str fontPath = renderObjectDef.font->GetHashName();
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
    renderObjectDef.localAABB = GetGameWorld()->GetRenderWorld()->GetTextMesh().Compute3DTextAABB(renderObjectDef.font, 
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
