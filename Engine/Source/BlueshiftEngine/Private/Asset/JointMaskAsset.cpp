#include "Precompiled.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("JointMask", JointMaskAsset, Asset)
BEGIN_EVENTS(JointMaskAsset)
END_EVENTS
BEGIN_PROPERTIES(JointMaskAsset)
END_PROPERTIES

JointMaskAsset::JointMaskAsset() {}

JointMaskAsset::~JointMaskAsset() {}

void JointMaskAsset::Reload() {
    /*JointMask *existingJointMask = jointMaskManager.FindJointMask(GetResourceFilename());
    if (existingJointMask) {
        existingJointMask->Reload();
        EmitSignal(&SIG_Reloaded);
    }*/
}

BE_NAMESPACE_END