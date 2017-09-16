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
#include "Asset/GuidMapper.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

GuidMapper  resourceGuidMapper;

// predefined texture GUID
Guid        GuidMapper::defaultTextureGuid(0, 0, 0, 100);
Guid        GuidMapper::zeroClampTextureGuid(0, 0, 0, 101);
Guid        GuidMapper::defaultCubeTextureGuid(0, 0, 0, 102);
Guid        GuidMapper::blackCubeTextureGuid(0, 0, 0, 103);
Guid        GuidMapper::whiteTextureGuid(0, 0, 0, 104);
Guid        GuidMapper::blackTextureGuid(0, 0, 0, 105);
Guid        GuidMapper::greyTextureGuid(0, 0, 0, 106);
Guid        GuidMapper::linearTextureGuid(0, 0, 0, 107);
Guid        GuidMapper::exponentTextureGuid(0, 0, 0, 108);
Guid        GuidMapper::flatNormalTextureGuid(0, 0, 0, 109);
Guid        GuidMapper::normalCubeTextureGuid(0, 0, 0, 110);
Guid        GuidMapper::cubicNormalCubeTextureGuid(0, 0, 0, 111);
Guid        GuidMapper::currentRenderTextureGuid(0, 0, 0, 112);
// predefined shader GUID
Guid        GuidMapper::standardSpecularShaderGuid(0, 0, 0, 200);
Guid        GuidMapper::skyboxCubemapShaderGuid(0, 0, 0, 201);
Guid        GuidMapper::skyboxSixSidedShaderGuid(0, 0, 0, 202);
Guid        GuidMapper::standardShaderGuid(0, 0, 0, 203);
Guid        GuidMapper::simpleShaderGuid(0, 0, 0, 204);
Guid        GuidMapper::phongLightingShaderGuid(0, 0, 0, 205);
// predefined material GUID
Guid        GuidMapper::defaultMaterialGuid(0, 0, 0, 300);
Guid        GuidMapper::whiteMaterialGuid(0, 0, 0, 301);
Guid        GuidMapper::blendMaterialGuid(0, 0, 0, 302);
Guid        GuidMapper::whiteLightMaterialGuid(0, 0, 0, 303);
Guid        GuidMapper::zeroClampLightMaterialGuid(0, 0, 0, 304);
Guid        GuidMapper::defaultSkyboxMaterialGuid(0, 0, 0, 305);
// predefined font GUID
Guid        GuidMapper::defaultFontGuid(0, 0, 0, 350);
// predefined mesh GUID
Guid        GuidMapper::defaultMeshGuid(0, 0, 0, 400);
Guid        GuidMapper::quadMeshGuid(0, 0, 0, 401);
Guid        GuidMapper::planeMeshGuid(0, 0, 0, 402);
Guid        GuidMapper::boxMeshGuid(0, 0, 0, 403);
Guid        GuidMapper::sphereMeshGuid(0, 0, 0, 404);
Guid        GuidMapper::geoSphereMeshGuid(0, 0, 0, 405);
Guid        GuidMapper::cylinderMeshGuid(0, 0, 0, 406);
Guid        GuidMapper::capsuleMeshGuid(0, 0, 0, 407);
// predefined particle system GUID
Guid        GuidMapper::defaultParticleSystemGuid(0, 0, 0, 500);
// predefined skeleton GUID
Guid        GuidMapper::defaultSkeletonGuid(0, 0, 0, 600);
// predefined anim controller GUID
Guid        GuidMapper::defaultAnimControllerGuid(0, 0, 0, 700);
// predefined sound GUID
Guid        GuidMapper::defaultSoundGuid(0, 0, 0, 800);

bool GuidMapper::Read(const char *filename) {
    File *file = fileSystem.OpenFileRead(filename, false);
    if (!file) {
        return false;
    }

    int32_t count;
    file->ReadInt32(count);

    for (int i = 0; i < count; i++) {
        Guid guid;
        Str assetPath;

        file->ReadGuid(guid);
        file->ReadString(assetPath);

        Set(guid, assetPath);
    }

    fileSystem.CloseFile(file);
    return true;
}

bool GuidMapper::Write(const char *filename) {
    File *file = fileSystem.OpenFileWrite(filename);
    if (!file) {
        return false;
    }

    int32_t count = Count();
    file->WriteInt32(count);

    const auto &kvArray = guidToFilePathMap.GetPairs();

    for (int i = 0; i < count; i++) {
        const auto &kv = kvArray[i];
        const Guid &guid = kv.first;

        file->WriteGuid(guid);
        file->WriteString(kv.second);
    }

    fileSystem.CloseFile(file);
    return true;
}

BE_NAMESPACE_END
