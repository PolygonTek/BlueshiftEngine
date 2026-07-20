// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Containers/HashMap.h"
#include "Containers/StrArray.h"
#include "ShaderTemplate.h"

/*
---------------------------------------------------------------------------------------------------

    Shader Template Manager

---------------------------------------------------------------------------------------------------
*/

class ShaderTemplateManager {
    friend class ShaderTemplate;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    void                            ProcessPendingGlobalDefineChanges();

    SharedPtr<ShaderTemplate>       AllocShaderTemplate(const char *hashName);
    SharedPtr<ShaderTemplate>       FindShaderTemplate(const char *hashName) const;
    SharedPtr<ShaderTemplate>       GetShaderTemplate(const char *hashName);

    bool                            FindGlobalDefine(const char *define) const;
    void                            AddGlobalDefine(const char *define);
    void                            RemoveGlobalDefine(const char *define);
    uint32_t                        CopyGlobalDefines(BE1::StrArray &outDefines) const;

    SharedPtr<ShaderTemplate>       fullscreenImageShaderTemplate;
    SharedPtr<ShaderTemplate>       debugDrawShaderTemplate;
    SharedPtr<ShaderTemplate>       depthShaderTemplate;
    SharedPtr<ShaderTemplate>       unlitShaderTemplate;
    SharedPtr<ShaderTemplate>       litShaderTemplate;

private:
    void                            InitGlobalDefines();
    void                            CleanExpiredShaderTemplates();
    void                            CreateEngineShaderTemplates();
    void                            DestroyEngineShaderTemplates();

    BE1::StrIHashMap<WeakPtr<ShaderTemplate>> shaderTemplateHashMap;
    int                             expiredShaderTemplateHashMapCursor = 0;
    BE1::StrArray                   globalDefines;
    uint32_t                        globalDefineModCount = 0;
    uint32_t                        lastInvalidatedGlobalDefineModCount = 0;
    BE1::PlatformMutex *            globalDefinesMutex = nullptr;
};

extern ShaderTemplateManager        shaderTemplateManager;
