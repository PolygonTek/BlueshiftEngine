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

#include "Containers/Array.h"
#include "Containers/HashMap.h"
#include "Core/Delegate.h"
#include "Material.h"

/*
---------------------------------------------------------------------------------------------------

    Material Manager

---------------------------------------------------------------------------------------------------
*/

class MaterialManager {
    friend class Material;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Material>             AllocMaterial(const char *hashName);
    SharedPtr<Material>             FindMaterial(const char *hashName) const;
    SharedPtr<Material>             GetMaterial(const char *hashName);
    SharedPtr<Material>             GetSingleTextureMaterial(const char *textureFilename, Material::TextureHint hint = Material::TextureHint::None);

    uint32_t                        GetStreamingMaterialCount() const { return streamingMaterials.Count(); }
    void                            ApplyStreamingFeedback(const uint32_t *feedbackData, uint32_t feedbackCapacity);

    SharedPtr<Material>             defaultUnlitMaterial;
    SharedPtr<Material>             defaultLitMaterial;
    SharedPtr<Material>             whiteUnlitMaterial;
    SharedPtr<Material>             whiteLitMaterial;
    SharedPtr<Material>             blendUnlitMaterial;
    SharedPtr<Material>             blendLitMaterial;
    SharedPtr<Material>             defaultSkyboxMaterial;

private:
    void                            OnShaderTemplateReloaded(const ShaderTemplate *shaderTemplate, const BE1::Array<ShaderPropertyInfo> &oldProperties);
    void                            CleanExpiredMaterials();
    void                            CreateEngineMaterials();
    void                            DestroyEngineMaterials();
    void                            RefreshStreamingMaterial(const SharedPtr<Material> &material);
    void                            RegisterStreamingMaterial(const SharedPtr<Material> &material);
    void                            UnregisterStreamingMaterial(Material *material);

    BE1::StrIHashMap<WeakPtr<Material>> materialHashMap;
    int                             expiredMaterialHashMapCursor = 0;
    uint32_t                        nextMaterialIndex = 0;
    BE1::Array<WeakPtr<Material>>   streamingMaterials;
    BE1::DelegateHandle             shaderTemplateReloadedHandle;
};

extern MaterialManager              materialManager;
