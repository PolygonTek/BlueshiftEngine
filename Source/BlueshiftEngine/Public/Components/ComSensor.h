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

#include "Physics/Physics.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComSensor : public Component {
public:
    OBJECT_PROTOTYPE(ComSensor);

    ComSensor();
    virtual ~ComSensor();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    ProcessScriptCallback();
    void                    TransformUpdated(const ComTransform *transform);

    PhysSensor *            sensor;
    PhysCollidableDesc      physicsDesc;
    Array<Guid>             oldColliders;
};

BE_NAMESPACE_END
