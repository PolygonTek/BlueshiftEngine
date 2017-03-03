#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Script", ScriptAsset, Asset)
BEGIN_EVENTS(ScriptAsset)
END_EVENTS
BEGIN_PROPERTIES(ScriptAsset)
END_PROPERTIES

ScriptAsset::ScriptAsset() {}

ScriptAsset::~ScriptAsset() {}

void ScriptAsset::Reload() {
    EmitSignal(&SIG_Reloaded);
}

BE_NAMESPACE_END