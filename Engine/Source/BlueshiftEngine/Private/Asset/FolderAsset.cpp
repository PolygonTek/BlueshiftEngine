#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Folder", FolderAsset, Asset)
BEGIN_EVENTS(FolderAsset)
END_EVENTS
BEGIN_PROPERTIES(FolderAsset)
END_PROPERTIES

FolderAsset::FolderAsset() {}

FolderAsset::~FolderAsset() {}

BE_NAMESPACE_END