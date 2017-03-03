#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("FBX", FbxAsset, Asset)
BEGIN_EVENTS(FbxAsset)
END_EVENTS
BEGIN_PROPERTIES(FbxAsset)
END_PROPERTIES

FbxAsset::FbxAsset() {
    isRedundantAsset = true;
}

FbxAsset::~FbxAsset() {}

BE_NAMESPACE_END