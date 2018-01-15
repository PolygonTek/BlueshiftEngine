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

/*
-------------------------------------------------------------------------------

    Map Render Settings

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class GameWorld;

class MapRenderSettings : public Object {
    friend class GameWorld;

public:
    OBJECT_PROTOTYPE(MapRenderSettings);

    MapRenderSettings();
    virtual ~MapRenderSettings();

    void                        Purge();

    void                        Init();

    Guid                        GetSkyboxMaterialGuid() const;
    void                        SetSkyboxMaterialGuid(const Guid &materialGuid);

protected:
    void                        ChangeSkyboxMaterial(const Guid &materialGuid);

    GameWorld *                 gameWorld;
    Material *                  skyboxMaterial;
    bool                        initialized;
};

BE_NAMESPACE_END
