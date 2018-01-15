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

class AssetDatabase;

BE_NAMESPACE_BEGIN

class Asset;

class AssetImporter : public Object {
    friend class ::AssetDatabase;

public:
    ABSTRACT_PROTOTYPE(AssetImporter);

    AssetImporter();
    virtual ~AssetImporter() = 0;

    const Asset *           GetAsset() const { return asset;  }

    virtual Str             ToString() const override;

    virtual Str             GetCacheFilename() const = 0;

    virtual void            Import() = 0;
    
    virtual void            ApplyChanged();

    void                    RevertChanged();

    static const SignalDef  SIG_ApplyChanged;

protected:
    Asset *                 asset;
};

BE_NAMESPACE_END