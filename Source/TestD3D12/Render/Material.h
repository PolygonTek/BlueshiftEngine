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

class MaterialManager;

/*
-------------------------------------------------------------------------------

    Material

-------------------------------------------------------------------------------
*/

class Material : public std::enable_shared_from_this<Material> {
    friend class MaterialManager;

public:
    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    bool                            Load(const char *filename);
    bool                            Reload();

private:
    BE1::Str                        hashName;
    BE1::Str                        name;
    uint32_t                        index = 0;
};

class MaterialManager {
    friend class Material;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    std::shared_ptr<Material>       AllocMaterial(const char *hashName);
    std::shared_ptr<Material>       FindMaterial(const char *hashName) const;
    std::shared_ptr<Material>       GetMaterial(const char *hashName);

    std::shared_ptr<Material>       defaultMaterial;
    std::shared_ptr<Material>       whiteMaterial;
    std::shared_ptr<Material>       unlitMaterial;

private:
    void                            CleanExpiredMaterials();

    BE1::StrIHashMap<std::weak_ptr<Material>> materialHashMap;
    int                             expiredMaterialHashMapCursor = 0;
    uint32_t                        nextMaterialIndex = 0;
};

extern MaterialManager              materialManager;
