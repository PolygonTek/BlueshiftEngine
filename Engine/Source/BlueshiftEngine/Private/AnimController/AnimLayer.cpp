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
#include "AnimController/AnimController.h"
#include "Core/Guid.h"

BE_NAMESPACE_BEGIN

AnimLayer::AnimLayer(AnimController *animController) {
    this->animController    = animController;
    this->defaultStateNum   = -1;
}

AnimLayer::AnimLayer(AnimController *animController, const AnimLayer *animLayer) {
    this->animController    = animController;
    this->name              = animLayer->name;
    this->maskJoints        = animLayer->maskJoints;
    this->blending          = animLayer->blending;
    this->weight            = animLayer->weight;

    for (int i = 0; i < stateHashMap.Count(); i++) {
        const auto *entry = stateHashMap.GetByIndex(i);
        AnimState *otherState = entry->second;
        AnimState *state = new AnimState(this, otherState);
        stateHashMap.Set(state->name, state);
    }
    this->defaultStateNum = animLayer->defaultStateNum;
    
    for (int i = 0; i < blendTrees.Count(); i++) {
        AnimBlendTree *otherBlendTree = blendTrees[i];
        AnimBlendTree *tree = new AnimBlendTree(this, otherBlendTree);
    }
    this->nodes = animLayer->nodes;
    this->leafs = animLayer->leafs;
}

AnimLayer::~AnimLayer() {
    FreeData();
}

void AnimLayer::FreeData() {
    maskJoints.Clear();
    stateHashMap.DeleteContents(true);
    blendTrees.DeleteContents(true);
    nodes.DeleteContents(true);
    leafs.DeleteContents(true);
    transitions.DeleteContents(true);
    defaultStateNum = 0;
}

size_t AnimLayer::Allocated() const {
    return name.Allocated() + maskJoints.Allocated() + stateHashMap.Allocated() + blendTrees.Allocated() + nodes.Allocated() + leafs.Allocated() + transitions.Allocated();
}

size_t AnimLayer::Size() const {
    return sizeof(*this) + Allocated();
}

AnimBlendTree *AnimLayer::CreateBlendTree(const char *name) {
    AnimBlendTree *blendTree = new AnimBlendTree(this, INVALID_ANIM_NODE);
    blendTree->SetName(name);

    int index = blendTrees.FindNull();
    if (index >= 0) {
        blendTrees[index] = blendTree;
    } else {
        index = blendTrees.Append(blendTree);
    }

    return blendTree;
}

void AnimLayer::DeleteBlendTree(AnimBlendTree *animBlendTree) {
    int index = blendTrees.FindIndex(animBlendTree);
    if (index < 0) {
        assert(0);
        return;
    }
    delete blendTrees[index];
    blendTrees.RemoveIndexFast(index);
}

int32_t AnimLayer::CreateNode(const Vec3 &blendSpaceVector, AnimBlendTree *animBlendTree) {
    int index = nodes.FindNull();
    if (index == -1) {
        index = nodes.Append(nullptr);
    }
    nodes[index] = new AnimNode;

    animBlendTree->SetNodeNum(index);

    AnimNode *childNode = nodes[index];
    childNode->blendSpaceVector = blendSpaceVector;
    childNode->animBlendTree    = animBlendTree;

    // return node num
    return index;
}

int32_t AnimLayer::CreateLeaf(const Vec3 &blendSpaceVector, AnimClip *animClip) {
    int index = leafs.FindNull();
    if (index == -1) {
        index = leafs.Append(nullptr);
    }
    leafs[index] = new AnimLeaf;

    AnimLeaf *childLeaf = leafs[index];
    childLeaf->blendSpaceVector = blendSpaceVector;
    childLeaf->animClip         = animClip;

    // return leaf num which is negative number
    return -(index + 1);
}

void AnimLayer::RemoveNode(int32_t nodeNum) {
    if (IS_ANIM_LEAF(nodeNum)) {
        int32_t leafNum = TO_ANIM_LEAFNUM(nodeNum);
        delete leafs[leafNum];
        leafs[leafNum] = nullptr;
    } else {
        delete nodes[nodeNum];
        nodes[nodeNum] = nullptr;
    }
}

AnimLayer::AnimNode *AnimLayer::GetNode(int32_t nodeNum) {
    if (nodeNum < 0 || nodeNum >= nodes.Count()) {
        assert(0);
        return nullptr;
    }

    return nodes[nodeNum];
}

AnimLayer::AnimLeaf *AnimLayer::GetLeaf(int32_t nodeNum) {
    int32_t leafNum = TO_ANIM_LEAFNUM(nodeNum);
    if (leafNum < 0 || leafNum >= leafs.Count()) {
        assert(0);
        return nullptr;
    }

    return leafs[leafNum];
}

const Vec3 AnimLayer::GetNodeBlendSpaceVector(int32_t nodeNum) const {
    if (IS_ANIM_LEAF(nodeNum)) {
        int leafNum = TO_ANIM_LEAFNUM(nodeNum);
        return leafs[leafNum]->blendSpaceVector;
    }
    return nodes[nodeNum]->blendSpaceVector;
}

void AnimLayer::SetNodeBlendSpaceVector(int32_t nodeNum, const Vec3 &blendSpaceVector) {
    if (IS_ANIM_LEAF(nodeNum)) {
        int leafNum = TO_ANIM_LEAFNUM(nodeNum);
        leafs[leafNum]->blendSpaceVector = blendSpaceVector;
        return;
    }
    nodes[nodeNum]->blendSpaceVector = blendSpaceVector;
}

AnimBlendTree *AnimLayer::GetNodeAnimBlendTree(int32_t nodeNum) const {
    if (IS_ANIM_LEAF(nodeNum)) {
        assert(0);
        return nullptr;
    }

    return nodes[nodeNum]->animBlendTree;
}

AnimClip *AnimLayer::GetNodeAnimClip(int32_t nodeNum) const {
    if (IS_ANIM_NODE(nodeNum)) {
        assert(0);
        return nullptr;
    }

    int leafNum = TO_ANIM_LEAFNUM(nodeNum);
    return leafs[leafNum]->animClip;
}

AnimState *AnimLayer::FindState(const char *name) const {
    const auto *entry = stateHashMap.Get(Str(name));
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

AnimState *AnimLayer::CreateState(const char *name) {
    AnimState *state = new AnimState(this);
    state->SetName(name);
    stateHashMap.Set(state->name, state);
    return state;
}

void AnimLayer::DeleteState(AnimState *state) {
    /*for (int i = 0; i < stateHashMap.Count(); i++) {
        const auto *entry = stateHashMap.GetByIndex(i);
        AnimState *otherState = entry->second;
        
        for (int j = 0; j < otherState->transitions.Count(); j++) {
            AnimTransition *trans = &otherState->transitions[j];
            if (trans->dstStateName == name) {
                otherState->RemoveTransition(*state);
                break;
            }
        }
    }*/
        
    stateHashMap.Remove(state->name);
    delete state;
}

const AnimState *AnimLayer::GetDefaultState() const {
    if (defaultStateNum < 0 || defaultStateNum >= stateHashMap.Count()) {
        BE_WARNLOG(L"AnimLayer::GetDefaultState: stateNum out of range\n");
        return nullptr;
    }
    
    return stateHashMap.GetPairs()[defaultStateNum].second;
}

void AnimLayer::SetDefaultState(const AnimState *state) {
    const auto *kv = stateHashMap.Get(state->name);
    if (kv) {
        defaultStateNum = stateHashMap.GetPairs().IndexOf(kv);
    }
}

const AnimLayer::AnimTransition *AnimLayer::FindTransition(const char *srcStateName, const char *dstStateName) const {
    for (int i = 0; i < transitions.Count(); i++) {
        const AnimTransition *transition = transitions[i];

        if (!Str::Cmp(transition->srcState->GetName(), srcStateName) && 
            !Str::Cmp(transition->dstState->GetName(), dstStateName)) {
            return transition;
        }
    }

    return nullptr;
}

bool AnimLayer::ListTransitionsFrom(const char *srcStateName, Array<const AnimTransition *> &transitionArray) const {
    for (int i = 0; i < transitions.Count(); i++) {
        const AnimTransition *transition = transitions[i];

        if (!Str::Cmp(transition->srcState->GetName(), srcStateName)) {
            transitionArray.Append(transition);
        }
    }

    return transitionArray.Count() > 0;
}

AnimLayer::AnimTransition *AnimLayer::CreateTransition(const char *srcStateName, const char *dstStateName) {
    if (FindTransition(srcStateName, dstStateName)) {
        BE_LOG(L"AnimLayer::CreateTransition: same transition exist\n");
        return nullptr;
    }

    AnimState *srcState = FindState(srcStateName);
    assert(srcState);
    AnimState *dstState = FindState(dstStateName);
    assert(dstState);

    AnimTransition *transition = new AnimTransition;
    transition->srcState = srcState;
    transition->dstState = dstState;
    transition->hasExitTime = false;
    transition->exitTime = 0;
    transition->fixedDuration = true;
    transition->duration = 0.25f;
    transition->startTime = 0;
    transition->isAtomic = false;
    
    int index = transitions.Append(transition);

    return transitions[index];
}

bool AnimLayer::RemoveTransition(AnimLayer::AnimTransition *transition) {
    return transitions.Remove(transition);
}

bool AnimLayer::ParseMaskJoints(Lexer &lexer) {
    Str token;

    if (!lexer.CheckTokenString("(")) {
        lexer.Warning("Expected '('\n");
        return false;
    }

    Str	jointnames = "";

    while (!lexer.CheckTokenString(")")) {
        if (!lexer.ReadToken(&token)) {
            lexer.Warning("Unexpected end of file");
            return false;
        }

        jointnames += token;
        if ((token != "*") && (token != "-")) {
            jointnames += " ";
        }
    }

    Array<int> jointNumArray;
    animController->GetJointNumListByString(jointnames, jointNumArray);

    maskJoints.SetCount(jointNumArray.Count());

    int num = 0;
    for (int i = 0; i < jointNumArray.Count(); i++) {
        maskJoints[num++] = jointNumArray[i];
    }

    return true;
}

bool AnimLayer::ParseState(Lexer &lexer) {
    Str token;
    Str token2;
    Str type;

    if (!lexer.ReadToken(&token)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    AnimState *state = CreateState(token.c_str());

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "animClip") {
            if (!lexer.ReadToken(&token2)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            const Guid animGuid = Guid::FromString(token2);
            AnimClip *animClip = animController->LoadAnimClip(animGuid);
            
            state->SetAnimClip(animClip);
        } else if (token == "blendTree") {
            // parse name
            if (!lexer.ReadToken(&token2)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            // parse type
            if (!lexer.ReadToken(&type)) {
                lexer.Warning("Unexpected end of file");
                return false;
            } 

            AnimBlendTree *blendTree = CreateBlendTree(token2.c_str());
            state->SetBlendTree(blendTree);

            //blendTree = GetNodeAnimBlendTree(state->nodeNum);
            if (!type.Icmp("blendAngle")) {
                blendTree->SetBlendType(AnimBlendTree::BlendType::BlendAngle);
            } else if (!type.Icmp("blend1D")) {
                blendTree->SetBlendType(AnimBlendTree::BlendType::Blend1D);
            } else if (!type.Icmp("blend2DDirectional")) {
                blendTree->SetBlendType(AnimBlendTree::BlendType::Blend2DDirectional);
            } else if (!type.Icmp("blend2DBarycentric")) {
                blendTree->SetBlendType(AnimBlendTree::BlendType::Blend2DBarycentric);
            } else if (!type.Icmp("blend3DBarycentric")) {
                blendTree->SetBlendType(AnimBlendTree::BlendType::Blend3DBarycentric);
            } else {
                lexer.Warning("Invalid blend type %s", type.c_str());
                return false;
            }

            if (!ParseBlendTree(lexer, blendTree)) {
                return false;
            }
        } else if (token == "default") {
            defaultStateNum = stateHashMap.Count() - 1;
        } else if (token == "position") {
            Vec2 position;
            position.x = lexer.ParseNumber();
            position.y = lexer.ParseNumber();
            state->SetNodePosition(position);
        } else if (token == "events") {
            if (!ParseEvents(lexer, state)) {
                return false;
            }
        } else {
            lexer.Warning("unknown token '%hs'", token.c_str());
            return false;
        }
    }

    return true;
}

bool AnimLayer::ParseBlendTree(Lexer &lexer, AnimBlendTree *blendTree) {
    Str token;
    Str token2;
    Str type;

    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "parameter") { // blendTree 가 참조하는 parameter
            switch (blendTree->GetBlendType()) {
            case AnimBlendTree::BlendAngle:
            case AnimBlendTree::Blend1D:
                if (!lexer.ReadToken(&token2)) {
                    lexer.Warning("Unexpected end of file");
                    return false;
                }

                blendTree->SetParameterIndex(0, animController->FindParameterIndex(token2.c_str()));
                break;
            case AnimBlendTree::Blend2DDirectional:
            case AnimBlendTree::Blend2DBarycentric:
                for (int i = 0; i < 2; i++) {
                    if (!lexer.ReadToken(&token2)) {
                        lexer.Warning("Unexpected end of file");
                        return false;
                    }

                    blendTree->SetParameterIndex(i, animController->FindParameterIndex(token2.c_str()));
                }
                break;
            case AnimBlendTree::Blend3DBarycentric:
                for (int i = 0; i < 3; i++) {
                    if (!lexer.ReadToken(&token2)) {
                        lexer.Warning("Unexpected end of file");
                        return false;
                    }

                    blendTree->SetParameterIndex(i, animController->FindParameterIndex(token2.c_str()));
                }
                break;
            }
            
        } else if (token == "animClip") {
            // blendTree 내부의 animClip 은 blend space point 를 갖는다
            Vec3 blendSpacePoint = Vec3::zero;
            switch (blendTree->GetBlendType()) {
            case AnimBlendTree::BlendType::BlendAngle:
            case AnimBlendTree::BlendType::Blend1D:
                lexer.Parse1DMatrix(1, (float *)blendSpacePoint);
                break;
            case AnimBlendTree::BlendType::Blend2DDirectional:
            case AnimBlendTree::BlendType::Blend2DBarycentric:
                lexer.Parse1DMatrix(2, (float *)blendSpacePoint);
                break;
            case AnimBlendTree::BlendType::Blend3DBarycentric:
                lexer.Parse1DMatrix(3, (float *)blendSpacePoint);
                break;
            }

            if (!lexer.ReadToken(&token2)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            const Guid animGuid = Guid::FromString(token2);
            AnimClip *animClip = animController->LoadAnimClip(animGuid);

            blendTree->AddChildClip(animClip, blendSpacePoint);
        } else if (token == "blendTree") {
            // blendTree 내부의 blendTree 역시 blend space point 를 갖는다.
            Vec3 blendSpacePoint = Vec3::zero;
            switch (blendTree->GetBlendType()) {
            case AnimBlendTree::BlendAngle:
            case AnimBlendTree::Blend1D:
                lexer.Parse1DMatrix(1, (float *)blendSpacePoint);
                break;
            case AnimBlendTree::Blend2DDirectional:
                lexer.Parse1DMatrix(2, (float *)blendSpacePoint);
                break;
            case AnimBlendTree::Blend2DBarycentric:
                lexer.Parse1DMatrix(2, (float *)blendSpacePoint);
                break;            
            case AnimBlendTree::Blend3DBarycentric:
                lexer.Parse1DMatrix(3, (float *)blendSpacePoint);
                break;
            }

            // Parse blend tree name
            if (!lexer.ReadToken(&token2)) { 
                lexer.Warning("Unexpected end of file");
                return false;
            }

            AnimBlendTree *subBlendTree = CreateBlendTree(token2.c_str());
            int32_t subNodeNum = blendTree->AddChildBlendTree(subBlendTree, blendSpacePoint);
            if (subNodeNum == INVALID_ANIM_NODE) {
                lexer.Warning("Failed to add child blend tree '%hs'\n", token2.c_str());
                return false;
            }

            if (!lexer.ReadToken(&type)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            if (!type.Icmp("blendAngle")) {
                subBlendTree->SetBlendType(AnimBlendTree::BlendType::BlendAngle);
            } else if (!type.Icmp("blend1D")) {
                subBlendTree->SetBlendType(AnimBlendTree::BlendType::Blend1D);
            } else if (!type.Icmp("blend2DDirectional")) {
                subBlendTree->SetBlendType(AnimBlendTree::BlendType::Blend2DDirectional);
            } else if (!type.Icmp("blend2DBarycentric")) {
                subBlendTree->SetBlendType(AnimBlendTree::BlendType::Blend2DBarycentric);
            } else if (!type.Icmp("blend3DBarycentric")) {
                subBlendTree->SetBlendType(AnimBlendTree::BlendType::Blend3DBarycentric);
            } else {
                lexer.Warning("Invalid blend type %s", type.c_str());
                return false;
            }

            if (!ParseBlendTree(lexer, subBlendTree)) {
                return false;
            }
        } else {
            lexer.Warning("Unknown token '%hs'", token.c_str());
            return false;
        }
    }

    return true;
}

bool AnimLayer::ParseEvents(Lexer &lexer, AnimState *state) {
    Str token;

    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        float time = lexer.ParseFloat();
        state->AddTimeEvent(time, token);
    }

    return true;
}

bool AnimLayer::ParseTransition(Lexer &lexer) {
    Str token, srcStateName, dstStateName;

    if (!lexer.ReadToken(&srcStateName)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    if (!lexer.ReadToken(&dstStateName)) {
        lexer.Warning("Unexpected end of file");
        return false;
    }

    if (!lexer.CheckTokenString("{")) {
        lexer.Warning("Expected { after '%hs'\n", token.c_str());
        return false;
    }

    AnimTransition *transition = CreateTransition(srcStateName, dstStateName);

    while (1) {
        if (!lexer.ReadToken(&token)) {
            break;
        }

        if (!token.Icmp("}")) {
            break;
        }

        if (token == "atomic") {
            transition->isAtomic = true;
        } else if (token == "hasExitTime") {
            transition->hasExitTime = true;
        } else if (token == "fixedDuration") {
            transition->fixedDuration = true;
        } else if (token == "exitTime") {
            transition->exitTime = lexer.ParseNumber();
        } else if (token == "startTime") {
            transition->startTime = lexer.ParseNumber();
        } else if (token == "duration") {
            transition->duration = lexer.ParseNumber();
        } else if (token == "condition") {
            Condition condition;

            // Parse parameter name
            if (!lexer.ReadToken(&token)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            condition.parameterIndex = animController->FindParameterIndex(token);

            // Parse compare func
            if (!lexer.ReadToken(&token)) {
                lexer.Warning("Unexpected end of file");
                return false;
            }

            if (token == "GT") {
                condition.compareFunc = Condition::GreaterThan;
            } else if (token == "GE") {
                condition.compareFunc = Condition::GreaterEqual;
            } else if (token == "LT") {
                condition.compareFunc = Condition::LessThan;
            } else if (token == "LE") {
                condition.compareFunc = Condition::LessEqual;
            } else if (token == "EQ") {
                condition.compareFunc = Condition::Equal;
            }

            // Parse compare value
            condition.value = lexer.ParseNumber();

            transition->conditions.Append(condition);
        }
    }

    return true;
}

BE_NAMESPACE_END
