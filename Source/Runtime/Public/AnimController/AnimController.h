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

#pragma once

/*
---------------------------------------------------------------------------------

    AnimController

---------------------------------------------------------------------------------
*/

#include "AnimLayer.h"

BE_NAMESPACE_BEGIN

class AnimClip;
class AnimLayer;

struct JointInfo {
    int                         num;            ///< Joint number 
    int                         parentNum;      ///< Parent joint number, -1 means it has no parent
};

struct AnimParm {
    AnimParm(const char *name, float defaultValue) : name(name), defaultValue(defaultValue) { }
    AnimParm(const AnimParm *other) { this->name = other->name; this->defaultValue = other->defaultValue; }

    Str                         name;           ///< Parameter name
    float                       defaultValue;   ///< Default value
};

class AnimController {
    friend class AnimControllerManager;

public:
    AnimController();
    ~AnimController();

    const char *                GetHashName() const { return hashName; }

    bool                        IsDefaultAnimController() const;

    const Vec3 &                GetRootOffset() const { return offset; }

                                /// Sets up bind pose joint matrices
                                /// @param numJoints The address to which the number of joints will be written
                                /// @param jointMats The address to which the joint matrices will be written
    void                        BuildBindPoseMats(int *numJoints, Mat3x4 **jointMats) const;

                                /// Returns the skeleton
    const Skeleton *            GetSkeleton() const { return skeleton; }
                                /// Sets the skeleton
    void                        SetSkeleton(Skeleton *skeleton);
                                /// Returns the bind poses from the skeleton
    const JointPose *           GetBindPoses() const;
                                /// Returns the number of joints
    int                         NumJoints() const { return joints.Count(); }
                                /// Returns the joint info
    const Array<JointInfo> &    GetJoints() const { return joints; }
                                /// Returns the joint info with the given name
    const JointInfo *           GetJoint(const char *name) const;
                                /// Returns the joint info with the given index
    const JointInfo *           GetJoint(int jointIndex) const;
                                /// Returns the joint name with the given index
    const char *                GetJointName(int jointIndex) const;
                                /// Returns the joint parent indexes
    const int *                 GetJointParents() const { return jointParents.Ptr(); }
                                /// Gets the joint nums with the given joint names
    void                        GetJointNumListByString(const char *jointNames, Array<int> &jointNumArray) const;

                                /// Returns the number of animation parameters
    int                         NumParameters() const { return animParameters.Count(); }
                                /// Returns the animation parameter with the given index
    AnimParm *                  GetParameterByIndex(int index);
                                /// Returns the animation parameter with the given name
    AnimParm *                  GetParameter(const char *name);
                                /// Returns the animation parameter index with the given name
    int                         FindParameterIndex(const char *name) const;
                                /// Deletes the animation parameter
    bool                        DeleteParameter(AnimParm *animParm);
                                /// Creates the animation parameter
    AnimParm *                  CreateParameter(const char *name, float defaultValue);

                                /// Returns the total number of animation clips
    int                         NumAnimClips() const { return animClips.Count(); }
                                /// Returns the animation clip with the given clip index
    AnimClip *                  GetAnimClip(int animClipIndex) const;
                                /// Returns the animation clip index with the given animation clip pointer
    int                         FindAnimClipIndex(const AnimClip *animClip) const { return animClips.FindIndex(const_cast<AnimClip *>(animClip)); }
                                /// Loads the animation clip with the given GUID
    AnimClip *                  LoadAnimClip(const Guid &animGuid);
                                /// Releases the animation clip
    void                        ReleaseAnimClip(AnimClip *animClip);

                                /// Returns the number of layers including base layer
    int                         NumAnimLayers() const { return animLayers.Count(); }
                                /// Returns the layer with the given index. Base layer is always in index 0
    AnimLayer *                 GetAnimLayerByIndex(int index) const;
                                /// Returns the layer with the given name
    AnimLayer *                 GetAnimLayer(const char *name);
                                /// Returns the layer index
    int                         FindAnimLayerIndex(const char *name) const;
                                /// Deletes the layer
    bool                        DeleteAnimLayer(AnimLayer *animLayer);
                                /// Creates layer 
    AnimLayer *                 CreateAnimLayer(const char *name);
                               
    void                        Purge();

                                /// Creates from the text
    bool                        Create(const char *text);

    bool                        Load(const char *filename);
    bool                        Reload();

    void                        Write(const char *filename);   

private:
    void                        Copy(const AnimController *def);
    bool                        ParseSkeleton(Lexer &lexer);
    bool                        ParseParameter(Lexer &lexer);
    bool                        ParseBaseAnimLayer(Lexer &lexer);
    bool                        ParseAnimLayer(Lexer &lexer);

    Str                         hashName;
    Str                         name;
    mutable int                 refCount;
    bool                        permanence;

    Skeleton *                  skeleton;       ///< Skeleton
    Array<JointInfo>            joints;         ///< Joint informations
    Array<int>                  jointParents;   ///< Parent indexes for each joints

    Array<AnimParm *>           animParameters; ///< Animation parameters
    Array<AnimClip *>           animClips;      ///< Animation clips
    Array<AnimLayer *>          animLayers;     ///< Animation layers
 
    Vec3                        offset;         ///< Arbitrary offset which is defined in the model space
};

BE_INLINE AnimController::AnimController() {
    refCount = 0;
    permanence = false;
    skeleton = nullptr;
    offset.SetFromScalar(0);
}

BE_INLINE AnimController::~AnimController() {
    Purge();
}

/*
---------------------------------------------------------------------------------

    AnimController manager

---------------------------------------------------------------------------------
*/

class AnimControllerManager {
    friend class AnimController;

public:
    void                        Init();
    void                        Shutdown();

    AnimController *            AllocAnimController(const char *name);
    AnimController *            FindAnimController(const char *name) const;
    AnimController *            GetAnimController(const char *name);

    void                        ReleaseAnimController(AnimController *animController, bool immediateDestroy = false);
    void                        DestroyAnimController(AnimController *animController);
    void                        DestroyUnusedAnimControllers();

    void                        PrecacheAnimController(const char *name);

    void                        RenameAnimController(AnimController *animController, const Str &newName);

    static AnimController *     defaultAnimController;

private:
    StrIHashMap<AnimController *> animControllerHashMap;
};

extern AnimControllerManager    animControllerManager;

BE_NAMESPACE_END
