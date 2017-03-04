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

#include "ComMeshRenderer.h"

BE_NAMESPACE_BEGIN

class ComStaticMeshRenderer : public ComMeshRenderer {
public:
    OBJECT_PROTOTYPE(ComStaticMeshRenderer);

    ComStaticMeshRenderer();
    virtual ~ComStaticMeshRenderer();

    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Update() override;

protected:
    virtual void            MeshUpdated() override;

    void                    PropertyChanged(const char *classname, const char *propName);
};

BE_NAMESPACE_END
