#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

const SignalDef     SIG_ApplyChanged("applyChanged");
    
ABSTRACT_DECLARATION("AssetImporter", AssetImporter, Object)
BEGIN_EVENTS(AssetImporter)
END_EVENTS
BEGIN_PROPERTIES(AssetImporter)
END_PROPERTIES

AssetImporter::AssetImporter() {
    asset = nullptr;
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&AssetImporter::PropertyChanged);
}

AssetImporter::~AssetImporter() {
}

const Str AssetImporter::ToString() const {
    return asset->ToString();
}

void AssetImporter::RevertChanged() {
    Str metaFilename = Asset::GetMetaFilenameFromAssetPath(asset->GetAssetFilename());

    Json::Value metaDataValue;
    bool validRootNode = false;
    char *text;
    if (fileSystem.LoadFile(metaFilename, false, (void **)&text) > 0) {
        Json::Reader jsonReader;
        validRootNode = jsonReader.parse(text, metaDataValue);
    }

    if (validRootNode) {
        Json::Value importerValue = metaDataValue["importer"];
        props->Init(importerValue);
    }
}

void AssetImporter::ApplyChanged() {
    Import();

    asset->Reload();

    asset->WriteMetaDataFile();

    EmitSignal(&SIG_ApplyChanged);
}

void AssetImporter::PropertyChanged(const char *classname, const char *propName) {
}

BE_NAMESPACE_END