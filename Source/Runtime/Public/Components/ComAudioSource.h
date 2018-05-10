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

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class Sound;

class ComAudioSource : public Component {
public:
    OBJECT_PROTOTYPE(ComAudioSource);

    ComAudioSource();
    virtual ~ComAudioSource();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    void                    Play();
    void                    Stop();
    bool                    IsPlaying() const;

    Guid                    GetAudioClipGuid() const;
    void                    SetAudioClipGuid(const Guid &guid);

    float                   GetVolume() const { return volume; }
    void                    SetVolume(float volume);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    TransformUpdated(const ComTransform *transform);

    Sound *                 referenceSound;
    Sound *                 sound;
    bool                    playOnAwake;
    float                   minDistance;
    float                   maxDistance;
    float                   volume;
    bool                    spatial;
    bool                    looping;
};

BE_NAMESPACE_END
