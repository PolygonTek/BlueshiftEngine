#pragma once

class AssetDatabase;

BE_NAMESPACE_BEGIN

class Asset;

class AssetImporter : public BE1::Object {
    friend class ::AssetDatabase;

public:
    ABSTRACT_PROTOTYPE(AssetImporter);

    AssetImporter();
    virtual ~AssetImporter() = 0;

    const Asset *           GetAsset() const { return asset;  }

    virtual const BE1::Str  ToString() const override;

    virtual const BE1::Str  GetCacheFilename() const = 0;

    virtual void            Import() = 0;
    
    virtual void            ApplyChanged();

    void                    RevertChanged();

protected:
    void                    PropertyChanged(const char *classname, const char *propName);

    Asset *                 asset;
};

extern const BE1::SignalDef	SIG_ApplyChanged;

BE_NAMESPACE_END