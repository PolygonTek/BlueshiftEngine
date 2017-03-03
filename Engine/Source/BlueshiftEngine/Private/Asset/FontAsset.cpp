#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Font", FontAsset, Asset)
BEGIN_EVENTS(FontAsset)
END_EVENTS
BEGIN_PROPERTIES(FontAsset)
END_PROPERTIES

FontAsset::FontAsset() {}

FontAsset::~FontAsset() {}

BE_NAMESPACE_END