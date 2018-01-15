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

#include "Precompiled.h"
#include "Render/Skeleton.h"
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"
#include "Asset/GuidMapper.h"
#include "Core/Heap.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

AnimController *            AnimControllerManager::defaultAnimController;

AnimControllerManager       animControllerManager;

void AnimControllerManager::Init() {
    animControllerHashMap.Init(1024, 256, 256);

    defaultAnimController = AllocAnimController("_defaultAnimController");
    defaultAnimController->Create(va("{ skeleton \"%s\" }", GuidMapper::defaultSkeletonGuid.ToString()));
    defaultAnimController->permanence = true;
}

void AnimControllerManager::Shutdown() {
    for (int i = 0; i < animControllerHashMap.Count(); i++) {
        const auto *entry = animControllerManager.animControllerHashMap.GetByIndex(i);
        AnimController *animController = entry->second;
        
        animController->Purge();
    }

    animControllerHashMap.DeleteContents(true);
}

AnimController *AnimControllerManager::AllocAnimController(const char *hashName) {
    if (animControllerHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs animController already allocated", hashName);
    }

    AnimController *animController = new AnimController;
    animController->hashName = hashName;
    animController->name = hashName;
    animController->name.StripPath();
    animController->name.StripFileExtension();
    animController->refCount = 1;

    animControllerHashMap.Set(animController->hashName, animController);

    return animController;
}

void AnimControllerManager::DestroyAnimController(AnimController *animController) {
    if (animController->refCount > 1) {
        BE_WARNLOG(L"AnimControllerManager::DestroyAnimController: animController '%hs' has %i reference count\n", animController->hashName.c_str(), animController->refCount);
    }

    animControllerHashMap.Remove(animController->hashName);

    delete animController;
}

void AnimControllerManager::ReleaseAnimController(AnimController *animController, bool immediateDestroy) {
    if (animController->permanence) {
        return;
    }

    if (animController->refCount > 0) {
        animController->refCount--;
    }

    if (immediateDestroy && animController->refCount == 0) {
        DestroyAnimController(animController);
    }
}

void AnimControllerManager::DestroyUnusedAnimControllers() {
    Array<AnimController *> removeArray;

    for (int i = 0; i < animControllerHashMap.Count(); i++) {
        const auto *entry = animControllerHashMap.GetByIndex(i);
        AnimController *animController = entry->second;

        if (animController && !animController->permanence && animController->refCount == 0) {
            removeArray.Append(animController);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyAnimController(removeArray[i]);
    }
}

void AnimControllerManager::PrecacheAnimController(const char *name) {
    AnimController *ac = GetAnimController(name);
    ReleaseAnimController(ac);
}

void AnimControllerManager::RenameAnimController(AnimController *animController, const Str &newName) {
    const auto *entry = animControllerHashMap.Get(animController->hashName);
    if (entry) {
        animControllerHashMap.Remove(animController->hashName);

        animController->hashName = newName;
        animController->name = newName;
        animController->name.StripPath();
        animController->name.StripFileExtension();

        animControllerHashMap.Set(newName, animController);
    }
}

AnimController *AnimControllerManager::FindAnimController(const char *hashName) const {
    const auto *entry = animControllerHashMap.Get(hashName);
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

AnimController *AnimControllerManager::GetAnimController(const char *name) {
    AnimController *animController = FindAnimController(name);
    if (animController) {
        animController->refCount++;
        return animController;
    }

    animController = AllocAnimController(name);
    if (!animController->Load(name)) {
        DestroyAnimController(animController);
        return defaultAnimController;
    }

    return animController;
}

//-----------------------------------------------------------------------------------------------------------

bool AnimController::IsDefaultAnimController() const {
    return (this == animControllerManager.defaultAnimController ? true : false);
}

void AnimController::Purge() {
    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
        skeleton = nullptr;
    }

    animClips.DeleteContents(true);
    animLayers.DeleteContents(true);
    animParameters.DeleteContents(true);

    joints.Clear();
    jointParents.Clear();

    offset.SetFromScalar(0);
}

void AnimController::Copy(const AnimController *def) {
    Purge();

    def->skeleton->AddRefCount();
    skeleton = def->skeleton;

    animClips.SetCount(def->animClips.Count());
    for (int i = 0; i < animClips.Count(); i++) {
        animClips[i] = new AnimClip(def->animClips[i]);
    }

    animLayers.SetCount(def->animLayers.Count());
    for (int i = 0; i < animLayers.Count(); i++) {
        animLayers[i] = new AnimLayer(this, def->animLayers[i]);
    }

    animParameters.SetCount(def->animParameters.Count());
    for (int i = 0; i < animParameters.Count(); i++) {
        animParameters[i] = new AnimParm(def->animParameters[i]);
    }

    joints.SetCount(def->joints.Count());
    memcpy(joints.Ptr(), def->joints.Ptr(), def->joints.Count() * sizeof(joints[0]));

    jointParents.SetCount(def->jointParents.Count());
    memcpy(jointParents.Ptr(), def->jointParents.Ptr(), def->jointParents.Count() * sizeof(jointParents[0]));

    offset = def->offset;
}

bool AnimController::Load(const char *filename) {
    BE_LOG(L"Loading animcon '%hs'...\n", filename);

    char *data;
    int size = (int)fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(data, size, filename);

    if (!lexer.ExpectTokenString("animController")) {
        fileSystem.FreeFile(data);
        return false;
    }

    Str baseDir = filename;
    baseDir.StripFileName();
    Create(data + lexer.GetCurrentOffset());

    fileSystem.FreeFile(data);

    return true;
}

bool AnimController::Reload() {
    Str _hashName = hashName;
    return Load(_hashName);
}

const JointPose *AnimController::GetBindPoses() const {
    return skeleton->GetBindPoses();
}

const JointInfo *AnimController::GetJoint(const char *name) const {
    if (!skeleton) {
        return nullptr;
    }

    const Joint *joint = skeleton->GetJoints();
    for (int i = 0; i < joints.Count(); i++, joint++) {
        if (!joint->name.Icmp(name)) {
            return &joints[i];
        }
    }

    return nullptr;
}

const JointInfo *AnimController::GetJoint(int jointIndex) const {
    if ((jointIndex < 0) || (jointIndex > joints.Count())) {
        BE_FATALERROR(L"AnimController::GetJoint : joint index out of range");
    }

    return &joints[jointIndex];
}

const char *AnimController::GetJointName(int jointIndex) const {
    if (!skeleton) {
        return nullptr;
    }

    if ((jointIndex < 0) || (jointIndex > joints.Count())) {
        BE_FATALERROR(L"AnimController::GetJointName : joint index out of range");
    }

    const Joint *joint = skeleton->GetJoints();
    return joint[jointIndex].name.c_str();
}

// jointNames: ex) Bone01 Bone02 -Bone03 *Bone04
//                 add    add    sub     addChildren
void AnimController::GetJointNumListByString(const char *jointNames, Array<int> &jointNumArray) const {
    if (!skeleton) {
        return;
    }

    jointNumArray.Clear();

    int numJoints = skeleton->NumJoints();
    bool subtract;
    bool getChildren;

    // scan through list of joints and add each to the joint list
    const char *pos = jointNames;
    while (*pos) {
        while ((*pos != 0) && ::isspace(*pos)) {
            pos++;
        }

        if (!*pos) {
            break;
        }

        if (*pos == '-') {
            subtract = true;
            pos++;
        } else {
            subtract = false;
        }

        if (*pos == '*') {
            getChildren = true;
            pos++;
        } else {
            getChildren = false;
        }

        // copy joint name
        Str jointName = "";
        while ((*pos != 0) && !::isspace(*pos)) {
            jointName += *pos;
            pos++;
        }

        const JointInfo *joint = GetJoint(jointName);
        if (!joint) {
            BE_WARNLOG(L"Unknown joint '%hs' in '%hs' for skeleton '%hs'\n", jointName.c_str(), jointNames, skeleton->GetHashName());
            continue;
        }

        if (!subtract) {
            jointNumArray.AddUnique(joint->num);
        } else {
            jointNumArray.Remove(joint->num);
        }

        if (getChildren) {
            // include all joint's children
            const JointInfo *child = joint + 1;
            for (int i = joint->num + 1; i < numJoints; i++, child++) {
                if (child->parentNum < joint->num) {
                    break;
                }

                if (!subtract) {
                    jointNumArray.AddUnique(child->num);
                } else {
                    jointNumArray.Remove(child->num);
                }
            }
        }
    }

    jointNumArray.Sort();
}

AnimClip *AnimController::GetAnimClip(int index) const {
    if ((index < 0) || (index >= animClips.Count())) {
        return nullptr;
    }

    return animClips[index];
}

AnimClip *AnimController::LoadAnimClip(const Guid &animGuid) {
    const Str animPath = resourceGuidMapper.Get(animGuid);
    Anim *anim = animManager.GetAnim(animPath);
    if (!anim) {
        anim = animManager.GetDefaultAnim(va("_defaultAnim_%s", GetSkeleton()->GetHashName()), GetSkeleton());
    }

    if (!anim->CheckHierarchy(GetSkeleton())) {
        BE_ERRLOG(L"mismatch hierarchy '%hs' with skeleton '%hs'\n", anim->GetName(), GetSkeleton()->GetName());
        animManager.ReleaseAnim(anim);
        return nullptr;
    }

    // FIXME: hash search
    int animClipIndex = 0;
    while (animClipIndex < animClips.Count()) {
        if (!Str::Icmp(animClips[animClipIndex]->GetAnim()->GetHashName(), anim->GetHashName())) {
            break;
        }
        animClipIndex++;
    }

    AnimClip *animClip;
    if (animClipIndex < animClips.Count()) {
        animClip = animClips[animClipIndex];
        animClip->refCount++;
    } else {
        animClip = new AnimClip;
        animClip->refCount = 1;
        animClips.Append(animClip);

        animClip->SetAnim(anim);
        animClip->Load(animPath + ".clip");
    }

    return animClip;
}

void AnimController::ReleaseAnimClip(AnimClip *animClip) {
    if (animClip->refCount > 0) {
        animClip->refCount--;

        if (animClip->refCount == 0) {
            animClips.Remove(animClip);
            delete animClip;
        }
    }
}

AnimParm *AnimController::GetParameterByIndex(int index) {
    if (index < 0 || index >= animParameters.Count()) {
        return nullptr;
    }

    return animParameters[index];
}

AnimParm *AnimController::GetParameter(const char *name) {
    int index = FindParameterIndex(name);
    return GetParameterByIndex(index);
}

int AnimController::FindParameterIndex(const char *name) const {
    for (int i = 0; i < animParameters.Count(); i++) {
        if (animParameters[i]->name == name) {
            return i;
        }
    }

    return -1;
}

bool AnimController::DeleteParameter(AnimParm *animParm) {
    int index = animParameters.FindIndex(animParm);
    if (index == -1) {
        return false;
    }
    delete animParameters[index];
    return animParameters.RemoveIndex(index);
}

AnimParm *AnimController::CreateParameter(const char *name, float defaultValue) {
    AnimParm *animParm = new AnimParm(name, defaultValue);
    animParameters.Append(animParm);

    return animParameters[animParameters.Count() - 1];
}

AnimLayer *AnimController::GetAnimLayerByIndex(int index) const {
    if ((index < 0) || (index >= animLayers.Count())) {
        return nullptr;
    }

    return animLayers[index]; 
}

AnimLayer *AnimController::GetAnimLayer(const char *name) {
    int index = FindAnimLayerIndex(name);
    return GetAnimLayerByIndex(index);
}

int AnimController::FindAnimLayerIndex(const char *name) const {
    for (int i = 0; i < animLayers.Count(); i++) {
        if (animLayers[i]->name == name) {
            return i;
        }
    }

    return -1;
}

bool AnimController::DeleteAnimLayer(AnimLayer *animLayer) {
    int index = animLayers.FindIndex(animLayer);
    if (index == -1) {
        return false;
    }
    delete animLayers[index];
    return animLayers.RemoveIndex(index);
}

AnimLayer *AnimController::CreateAnimLayer(const char *name) {
    AnimLayer *animLayer = new AnimLayer(this);
    animLayer->SetName(name);
    animLayers.Append(animLayer);

    return animLayer;
}

void AnimController::BuildBindPoseMats(int *numJoints, Mat3x4 **jointMats) const {
    if (!skeleton || skeleton->IsDefaultSkeleton()) {
        Mem_AlignedFree((*jointMats));
        (*jointMats) = nullptr;
        return;
    }

    int num = skeleton->NumJoints();
    if (!num) {
        BE_FATALERROR(L"skeleton '%hs' has no joints", skeleton->GetHashName());
    }
 
    // Set up initial pose for skeleton
    Mat3x4 *mats = (Mat3x4 *)Mem_Alloc16(num * sizeof(mats[0]));

    const JointPose *bindPoses = GetBindPoses();

    // Convert the joint quaternions to joint matrices (quaternions -> local joint matrices)
    simdProcessor->ConvertJointPosesToJointMats(mats, bindPoses, joints.Count());

    // Check if we offset the skeleton by the root joint
    mats[0].SetTranslation(bindPoses[0].t + offset);
    
    // Transform the joint hierarchy (local matrices -> world matrices)
    simdProcessor->TransformJoints(mats, jointParents.Ptr(), 1, joints.Count() - 1);

    *numJoints = num;
    *jointMats = mats;
}

bool AnimController::Create(const char *text) {
    Str token;
    Str token2;
    
    Purge();
    
    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName);

    lexer.SkipUntilString("{");

    int numDefaultAnims = 0;

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "skeleton") {
            if (!ParseSkeleton(lexer)) {
                return false;
            }
        } else if (token == "parameter") {
            if (!ParseParameter(lexer)) {
                return false;
            }	
        } else if (token == "baseLayer") {
            if (!ParseBaseAnimLayer(lexer)) {
                return false;
            }
        } else if (token == "animLayer") {
            if (!ParseAnimLayer(lexer)) {
                return false;
            }
        } else if (token == "offset") {
            if (!lexer.Parse1DMatrix(3, (float *)offset)) {
                lexer.Warning("Expected vector following 'offset'");
                return false;
            }
        } else {
            lexer.Warning("unknown token '%hs'", token.c_str());
            return false;
        }
    }

    // shrink animClips & animLayers down to save space
    animClips.SetGranularity(1);
    animClips.SetCount(animClips.Count());
    animLayers.SetGranularity(1);
    animLayers.SetCount(animLayers.Count());

    return true;
}

bool AnimController::ParseSkeleton(Lexer &lexer) {
    Str token;
    
    if (!lexer.ReadToken(&token)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    const Guid skeletonGuid = Guid::FromString(token);
    const Str skeletonPath = resourceGuidMapper.Get(skeletonGuid);
    
    skeleton = skeletonManager.GetSkeleton(skeletonPath);
    if (skeleton->IsDefaultSkeleton()) {
        lexer.Warning("Skeleton '%hs' defaulted", skeleton->GetHashName());
        return false;
    }
    
    SetSkeleton(skeleton);
    
    return true;
}

void AnimController::SetSkeleton(Skeleton *skeleton) {
    this->skeleton = skeleton;
    
    joints.SetGranularity(1);
    joints.SetCount(skeleton->NumJoints());

    // jointParents 는 SIMD 연산을 위해 따로 배열 형태로 저장한다.
    jointParents.SetGranularity(1);
    jointParents.SetCount(skeleton->NumJoints());

    const Joint *skeletonJoint = skeleton->GetJoints();

    for (int i = 0; i < skeleton->NumJoints(); i++, skeletonJoint++) {
        joints[i].num = i;

        if (skeletonJoint->parent) {
            joints[i].parentNum = static_cast<int>(skeletonJoint->parent - skeleton->GetJoints());
        } else {
            joints[i].parentNum = -1;
        }

        jointParents[i] = joints[i].parentNum;
    }
}

bool AnimController::ParseBaseAnimLayer(Lexer &lexer) {
    Str token;
    
    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    AnimLayer *animLayer = new AnimLayer(this);
    animLayer->SetName("Base Layer");
    animLayer->SetBlending(AnimLayer::Override);
    animLayer->SetWeight(1.0f);
    animLayer->maskJoints.SetCount(joints.Count());
    for (int i = 0; i < joints.Count(); i++) {
        animLayer->maskJoints[i] = joints[i].num;
    }
    // first layer should be a base layer
    animLayers.Append(animLayer);

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "state") {
            if (!animLayer->ParseState(lexer)) {
                return false;
            }
        } else if (token == "transition") {
            if (!animLayer->ParseTransition(lexer)) {
                return false;
            }
        } else {
            lexer.Warning("unknown token '%hs'", token.c_str());
            return false;
        }
    }

    return true;
}

bool AnimController::ParseAnimLayer(Lexer &lexer) {
    Str token;
    Str token2;

    if (!lexer.ReadToken(&token)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    AnimLayer *animLayer = new AnimLayer(this);
    animLayer->SetName(token.c_str());
    animLayers.Append(animLayer);

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "blending") {
            if (!lexer.ReadToken(&token2)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            if (token2 == "override") {
                animLayer->SetBlending(AnimLayer::Blending::Override);
            } else if (token2 == "additive") {
                animLayer->SetBlending(AnimLayer::Blending::Additive);
            } else {
                lexer.Warning("Unknown blending type '%hs'", token2.c_str());
                return false;
            }
        } else if (token == "weight") {
            float weight = lexer.ParseNumber();
            animLayer->SetWeight(weight);
        } else if (token == "mask") {
            if (!animLayer->ParseMaskJoints(lexer)) {
                return false;
            }
        } else if (token == "state") {
            if (!animLayer->ParseState(lexer)) {
                return false;
            }
        } else if (token == "transition") {
            if (!animLayer->ParseTransition(lexer)) {
                return false;
            }
        } else {
            lexer.Warning("unknown token '%hs'", token.c_str());
            return false;
        }
    }

    return true;
}

bool AnimController::ParseParameter(Lexer &lexer) {
    Str name;

    if (!lexer.ReadToken(&name)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    float defaultValue = lexer.ParseNumber();

    CreateParameter(name, defaultValue);

    return true;
}

void AnimController::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"AnimController::Write: file open error\n");
        return;
    }

    Str indentSpace;

    fp->Printf("animController {\n");
    indentSpace += "  ";

    // Write skeleton
    const Guid skeletonGuid = resourceGuidMapper.Get(skeleton->GetHashName());
    fp->Printf("%sskeleton \"%s\"\n", indentSpace.c_str(), skeletonGuid.ToString());

    // Write parameters
    for (int i = 0; i < animParameters.Count(); i++) {
        const AnimParm *animParm = animParameters[i];

        Str defaultValueStr = Str::FloatArrayToString((const float *)&animParm->defaultValue, 1, 3); 
        fp->Printf("%sparameter \"%s\" %s\n", indentSpace.c_str(), animParm->name.c_str(), defaultValueStr.c_str());
    }

    // Write layers
    if (animLayers.Count() > 0) {
        // Write base layer
        const AnimLayer *baseLayer = animLayers[0];

        fp->Printf("%sbaseLayer {\n", indentSpace.c_str());
        indentSpace += "  ";

        for (int stateIndex = 0; stateIndex < baseLayer->stateHashMap.Count(); stateIndex++) {
            const auto *entry = baseLayer->stateHashMap.GetByIndex(stateIndex);
            AnimState *animState = entry->second;

            fp->Printf("%sstate \"%s\" {\n", indentSpace.c_str(), animState->GetName().c_str());
            indentSpace += "  ";

            fp->Printf("%sposition %s\n", indentSpace.c_str(), animState->GetNodePosition().ToString(0));

            if (stateIndex == baseLayer->defaultStateNum) {
                fp->Printf("%sdefault\n", indentSpace.c_str());
            }
            
            if (animState->IsAnimBlendTree()) {
                const AnimBlendTree *blendTree = animState->GetAnimBlendTree();

                Str blendTypeStr;
                switch (blendTree->GetBlendType()) {
                case AnimBlendTree::BlendType::BlendAngle:
                    blendTypeStr = "blendAngle";
                    break;
                case AnimBlendTree::BlendType::Blend1D:
                    blendTypeStr = "blend1D";
                    break; 
                case AnimBlendTree::BlendType::Blend2DDirectional:
                    blendTypeStr = "blend2DDirectional";
                    break;
                case AnimBlendTree::BlendType::Blend2DBarycentric:
                    blendTypeStr = "blend2DBarycentric";
                    break;                 
                case AnimBlendTree::BlendType::Blend3DBarycentric:
                    blendTypeStr = "blend3DBarycentric";
                    break;
                }

                fp->Printf("%sblendTree \"%s\" %s {\n", indentSpace.c_str(), blendTree->GetName().c_str(), blendTypeStr.c_str());
                indentSpace += "  ";

                blendTree->Write(fp, indentSpace);

                indentSpace.Truncate(indentSpace.Length() - 2);
                fp->Printf("%s}\n", indentSpace.c_str());
            } else if (animState->IsAnimClip()) {
                const AnimClip *animClip = animState->GetAnimClip();
                const Guid animGuid = (!animClip || animClip->GetAnim()->IsDefaultAnim()) ? Guid() : resourceGuidMapper.Get(animClip->GetAnim()->GetHashName());

                fp->Printf("%sanimClip \"%s\"\n", indentSpace.c_str(), animGuid.ToString());
            } else {
                fp->Printf("%sanimClip \"%s\"\n", indentSpace.c_str(), Guid::zero.ToString());
            }

            if (animState->NumTimeEvents() > 0) {
                fp->Printf("%sevents {\n", indentSpace.c_str());
                indentSpace += "  ";

                for (int eventIndex = 0; eventIndex < animState->NumTimeEvents(); eventIndex++) {
                    AnimTimeEvent &event = animState->GetTimeEvent(eventIndex);

                    fp->Printf("%s\"%hs\" %f\n", indentSpace.c_str(), event.string.c_str(), event.time);
                }

                indentSpace.Truncate(indentSpace.Length() - 2);
                fp->Printf("%s}\n", indentSpace.c_str());
            }

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        for (int transitionIndex = 0; transitionIndex < baseLayer->transitions.Count(); transitionIndex++) {
            const AnimLayer::AnimTransition *transition = baseLayer->transitions[transitionIndex];

            fp->Printf("%stransition \"%s\" \"%s\" {\n", indentSpace.c_str(), transition->srcState->GetName().c_str(), transition->dstState->GetName().c_str());
            indentSpace += "  ";

            for (int conditionIndex = 0; conditionIndex < transition->conditions.Count(); conditionIndex++) {
                const AnimLayer::Condition *condition = &transition->conditions[conditionIndex];

                const AnimParm *parm = GetParameterByIndex(condition->parameterIndex);
                const char *parmName = parm->name.c_str();
                
                const char *compareFunc = "EQ";
                switch (condition->compareFunc) {
                case AnimLayer::Condition::CompareFunc::GreaterThan:    compareFunc = "GT"; break;
                case AnimLayer::Condition::CompareFunc::GreaterEqual:   compareFunc = "GE"; break;
                case AnimLayer::Condition::CompareFunc::LessThan:       compareFunc = "LT"; break;
                case AnimLayer::Condition::CompareFunc::LessEqual:      compareFunc = "LE"; break;
                case AnimLayer::Condition::CompareFunc::Equal:          compareFunc = "EQ"; break;
                }

                fp->Printf("%scondition \"%s\" %s %f\n", indentSpace.c_str(), parmName, compareFunc, condition->value);
            }

            if (transition->isAtomic) {
                fp->Printf("%satomic\n", indentSpace.c_str());
            }

            if (transition->hasExitTime) {
                fp->Printf("%shasExitTime\n", indentSpace.c_str());
            }

            if (transition->fixedDuration) {
                fp->Printf("%sfixedDuration\n", indentSpace.c_str());
            }

            fp->Printf("%sexitTime %f\n", indentSpace.c_str(), transition->exitTime);
            fp->Printf("%sstartTime %f\n", indentSpace.c_str(), transition->startTime);
            fp->Printf("%sduration %f\n", indentSpace.c_str(), transition->duration);

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());

        // Write additional anim layers
        for (int layerIndex = 1; layerIndex < animLayers.Count(); layerIndex++) {
            const AnimLayer *animLayer = animLayers[layerIndex];

            fp->Printf("%sanimLayer \"%s\" {\n", indentSpace.c_str(), animLayer->GetName().c_str());
            indentSpace += "  ";

            Str blendingStr;
            switch (animLayer->GetBlending()) {
            case AnimLayer::Override:
                blendingStr = "override";
                break;
            case AnimLayer::Additive:
                blendingStr = "additive";
                break;
            }
            fp->Printf("%sblending %s\n", indentSpace.c_str(), blendingStr.c_str());

            fp->Printf("%sweight %.4f\n", indentSpace.c_str(), animLayer->GetWeight());

            for (int stateIndex = 0; stateIndex < animLayer->stateHashMap.Count(); stateIndex++) {
                const auto *entry = animLayer->stateHashMap.GetByIndex(stateIndex);
                AnimState *animState = entry->second;

                fp->Printf("%sstate \"%s\" {\n", indentSpace.c_str(), animState->GetName().c_str());
                indentSpace += "  ";

                if (stateIndex == animLayer->defaultStateNum) {
                    fp->Printf("%sdefault\n", indentSpace.c_str());
                }

                if (animState->IsAnimBlendTree()) {
                    const AnimBlendTree *blendTree = animState->GetAnimBlendTree();

                    Str blendTypeStr;
                    switch (blendTree->GetBlendType()) {
                    case AnimBlendTree::BlendType::BlendAngle:
                        blendTypeStr = "blendAngle";
                        break;
                    case AnimBlendTree::BlendType::Blend1D:
                        blendTypeStr = "blend1D";
                        break; 
                    case AnimBlendTree::BlendType::Blend2DDirectional:
                        blendTypeStr = "blend2DDirectional";
                        break;
                    case AnimBlendTree::BlendType::Blend2DBarycentric:
                        blendTypeStr = "blend2DBarycentric";
                        break;                     
                    case AnimBlendTree::BlendType::Blend3DBarycentric:
                        blendTypeStr = "blend3DBarycentric";
                        break;
                    }

                    fp->Printf("%sblendTree \"%s\" %s {\n", indentSpace.c_str(), blendTree->GetName().c_str(), blendTypeStr.c_str());
                    indentSpace += "  ";

                    blendTree->Write(fp, indentSpace);

                    indentSpace.Truncate(indentSpace.Length() - 2);
                    fp->Printf("%s}\n", indentSpace.c_str());
                } else if (animState->IsAnimClip()) {
                    const AnimClip *animClip = animState->GetAnimClip();
                    const Guid animGuid = resourceGuidMapper.Get(animClip->GetAnim()->GetHashName());

                    fp->Printf("%sanimClip \"%s\"\n", indentSpace.c_str(), animGuid.ToString());
                } else {
                    fp->Printf("%sanimClip \"%s\"\n", indentSpace.c_str(), Guid::zero.ToString());
                }

                if (animState->NumTimeEvents() > 0) {
                    fp->Printf("%sevents {\n", indentSpace.c_str());
                    indentSpace += "  ";

                    for (int eventIndex = 0; eventIndex < animState->NumTimeEvents(); eventIndex++) {
                        AnimTimeEvent &event = animState->GetTimeEvent(eventIndex);

                        fp->Printf("%s\"%hs\" %f\n", indentSpace.c_str(), event.string.c_str(), event.time);
                    }

                    indentSpace.Truncate(indentSpace.Length() - 2);
                    fp->Printf("%s}\n", indentSpace.c_str());
                }

                indentSpace.Truncate(indentSpace.Length() - 2);
                fp->Printf("%s}\n", indentSpace.c_str());
            }

            for (int transitionIndex = 0; transitionIndex < animLayer->transitions.Count(); transitionIndex++) {
                const AnimLayer::AnimTransition *transition = animLayer->transitions[transitionIndex];

                fp->Printf("%stransition \"%s\" \"%s\" {\n", indentSpace.c_str(), transition->srcState->GetName().c_str(), transition->dstState->GetName().c_str());
                indentSpace += "  ";

                for (int conditionIndex = 0; conditionIndex < transition->conditions.Count(); conditionIndex++) {
                    const AnimLayer::Condition *condition = &transition->conditions[conditionIndex];

                    const AnimParm *parm = GetParameterByIndex(condition->parameterIndex);
                    const char *parmName = parm->name.c_str();

                    const char *compareFunc = "EQ";
                    switch (condition->compareFunc) {
                    case AnimLayer::Condition::CompareFunc::GreaterThan:    compareFunc = "GT"; break;
                    case AnimLayer::Condition::CompareFunc::GreaterEqual:   compareFunc = "GE"; break;
                    case AnimLayer::Condition::CompareFunc::LessThan:       compareFunc = "LT"; break;
                    case AnimLayer::Condition::CompareFunc::LessEqual:      compareFunc = "LE"; break;
                    case AnimLayer::Condition::CompareFunc::Equal:          compareFunc = "EQ"; break;
                    }

                    fp->Printf("%scondition \"%s\" %s %f\n", indentSpace.c_str(), parmName, compareFunc, condition->value);
                }

                if (transition->isAtomic) {
                    fp->Printf("%satomic\n", indentSpace.c_str());
                }

                if (transition->hasExitTime) {
                    fp->Printf("%shasExitTime\n", indentSpace.c_str());
                }

                if (transition->fixedDuration) {
                    fp->Printf("%sfixedDuration\n", indentSpace.c_str());
                }

                fp->Printf("%sexitTime %f\n", indentSpace.c_str(), transition->exitTime);
                fp->Printf("%sstartTime %f\n", indentSpace.c_str(), transition->startTime);
                fp->Printf("%sduration %f\n", indentSpace.c_str(), transition->duration);

                indentSpace.Truncate(indentSpace.Length() - 2);
                fp->Printf("%s}\n", indentSpace.c_str());
            }

            indentSpace.Truncate(indentSpace.Length() - 2);
            fp->Printf("%s}\n", indentSpace.c_str());
        }
    }

    indentSpace.Truncate(indentSpace.Length() - 2);
    fp->Printf("%s}\n", indentSpace.c_str());

    fileSystem.CloseFile(fp);
}

BE_NAMESPACE_END
