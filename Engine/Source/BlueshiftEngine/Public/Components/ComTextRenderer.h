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

#pragma once

#include "ComRenderable.h"

BE_NAMESPACE_BEGIN

class ComTextRenderer : public ComRenderable {
public:
    OBJECT_PROTOTYPE(ComTextRenderer);

    ComTextRenderer();
    virtual ~ComTextRenderer();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    const char *            GetTextCString() const;
    void                    SetTextCString(const char *text);

    Str                     GetText() const;
    void                    SetText(const Str &text);

    SceneEntity::TextAnchor GetAnchor() const;
    void                    SetAnchor(SceneEntity::TextAnchor anchor);

    SceneEntity::TextAlignment GetAlignment() const;
    void                    SetAlignment(SceneEntity::TextAlignment alignment);

    float                   GetLineSpacing() const;
    void                    SetLineSpacing(float lineSpacing);

    Guid                    GetFontGuid() const;
    void                    SetFontGuid(const Guid &fontGuid);

    int                     GetFontSize() const;
    void                    SetFontSize(int fontSize);

protected:
    void                    ChangeFont(const Guid &fontGuid, int fontSize);

    void                    UpdateAABB();

    int                     fontSize;
};

BE_NAMESPACE_END
