#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Map", MapAsset, Asset)
BEGIN_EVENTS(MapAsset)
END_EVENTS
BEGIN_PROPERTIES(MapAsset)
END_PROPERTIES

MapAsset::MapAsset() {}

MapAsset::~MapAsset() {}

BE_NAMESPACE_END