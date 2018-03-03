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

#include "Core/Str.h"
#include "Core/Guid.h"
#include "Containers/HashMap.h"

BE_NAMESPACE_BEGIN

class GuidMapper {
public:
                            /// Gets number of elements
    int                     Count() const { assert(guidToFilePathMap.Count() == filePathToGuidMap.Count()); return guidToFilePathMap.Count(); }

                            /// Converts GUID to path
    const Str               Get(const Guid &guid) const;

                            /// Converts path to GUID
    const Guid              Get(const Str &filePath) const;

                            /// Sets hash
    void                    Set(const Guid &guid, const Str &filePath);

                            /// Removes hash using GUID
    bool                    Remove(const Guid &guid);

                            /// Removes hash using path
    bool                    Remove(const Str &filePath);

                            /// Reads GUID map from file
    bool                    Read(const char *filename);

                            /// Writes GUID map to file
    bool                    Write(const char *filename);

    static Guid             defaultTextureGuid;
    static Guid             zeroClampTextureGuid;
    static Guid             defaultCubeTextureGuid;
    static Guid             blackCubeTextureGuid;
    static Guid             whiteTextureGuid;
    static Guid             blackTextureGuid;
    static Guid             greyTextureGuid;
    static Guid             linearTextureGuid;
    static Guid             exponentTextureGuid;
    static Guid             flatNormalTextureGuid;
    static Guid             normalCubeTextureGuid;
    static Guid             cubicNormalCubeTextureGuid;
    static Guid             currentRenderTextureGuid;

    static Guid             simpleShaderGuid;
    static Guid             standardSpecularShaderGuid;
    static Guid             standardShaderGuid;
    static Guid             phongLightingShaderGuid;
    static Guid             skyboxCubemapShaderGuid;
    static Guid             skyboxSixSidedShaderGuid;

    static Guid             defaultMaterialGuid;
    static Guid             whiteMaterialGuid;
    static Guid             blendMaterialGuid;
    static Guid             whiteLightMaterialGuid;
    static Guid             zeroClampLightMaterialGuid;
    static Guid             defaultSkyboxMaterialGuid;

    static Guid             defaultFontGuid;

    static Guid             defaultMeshGuid;
    static Guid             quadMeshGuid;
    static Guid             planeMeshGuid;
    static Guid             boxMeshGuid;
    static Guid             sphereMeshGuid;
    static Guid             geoSphereMeshGuid;
    static Guid             cylinderMeshGuid;
    static Guid             capsuleMeshGuid;

    static Guid             defaultParticleSystemGuid;

    static Guid             defaultSkeletonGuid;

    static Guid             defaultAnimControllerGuid;

    static Guid             defaultSoundGuid;

private:
    HashMap<Guid, Str>      guidToFilePathMap;
    HashMap<Str, Guid>      filePathToGuidMap;
};

BE_INLINE const Str GuidMapper::Get(const Guid &guid) const {
    const auto *kv = guidToFilePathMap.Get(guid);
    if (!kv) {
        return "";
    }

    return kv->second;
}

BE_INLINE const Guid GuidMapper::Get(const Str &filePath) const {
    const auto *kv = filePathToGuidMap.Get(filePath);
    if (!kv) {
        return Guid();
    }

    return kv->second;
}

BE_INLINE void GuidMapper::Set(const Guid &guid, const Str &filePath) {
    Remove(guid);

    guidToFilePathMap.Set(guid, filePath);
    filePathToGuidMap.Set(filePath, guid);
}

BE_INLINE bool GuidMapper::Remove(const Guid &guid) {
    const Str filePath = Get(guid);
    if (!filePath.IsEmpty()) {
        guidToFilePathMap.Remove(guid);
        filePathToGuidMap.Remove(filePath);
        return true;
    }

    return false;
}

BE_INLINE bool GuidMapper::Remove(const Str &filePath) {
    const Guid guid = Get(filePath);
    if (!guid.IsZero()) {
        filePathToGuidMap.Remove(filePath);
        guidToFilePathMap.Remove(guid);
        return true;
    }
    
    return false;
}

extern GuidMapper       resourceGuidMapper;

BE_NAMESPACE_END
