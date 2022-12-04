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

class ComRectTransform;

class ComImage : public ComRenderable {
public:
    OBJECT_PROTOTYPE(ComImage);

    struct ImageType {
        enum Enum {
            Simple,
            Sliced
        };
    };
    
    ComImage();
    virtual ~ComImage();

    virtual void            Purge(bool chainPurge) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    Guid                    GetMaterialGuid() const;
    void                    SetMaterialGuid(const Guid &guid);

    Material *              GetMaterial() const;
    void                    SetMaterial(const Material *material);

    ImageType::Enum         GetImageType() const;
    void                    SetImageType(ImageType::Enum imageType);

protected:
    virtual void            UpdateVisuals() override;

    void                    RectTransformUpdated(const ComRectTransform *rectTransform);
    void                    UpdateRawVertexes();

    ImageType::Enum         imageType = ImageType::Simple;
};

BE_NAMESPACE_END
