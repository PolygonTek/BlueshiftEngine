#include "Precompiled.h"
#include "Render/Anim.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Anim", AnimAsset, Asset)
BEGIN_EVENTS(AnimAsset)
END_EVENTS
BEGIN_PROPERTIES(AnimAsset)
END_PROPERTIES

AnimAsset::AnimAsset() {
    anim = nullptr;
}

AnimAsset::~AnimAsset() {
    if (!anim) {
        const Str animPath = resourceGuidMapper.Get(GetGuid());
        anim = animManager.FindAnim(animPath);
    }

    if (anim) {
        animManager.ReleaseAnim(anim, true);
    }
}

Anim *AnimAsset::GetAnim() {
    if (anim) {
        return anim;
    }
    const Str animPath = resourceGuidMapper.Get(GetGuid());
    anim = animManager.GetAnim(animPath);
    return anim;
}

void AnimAsset::Reload() {
    Anim *existingAnim = animManager.FindAnim(GetResourceFilename());
    if (existingAnim) {
        existingAnim->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END