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

#include "Containers/HashMap.h"
#include "ComLogic.h"
#include "Script/LuaVM.h"

BE_NAMESPACE_BEGIN

class Collision;
class ScriptAsset;

class ComScript : public ComLogic {
public:
    OBJECT_PROTOTYPE(ComScript);

    ComScript();
    virtual ~ComScript();

                            /// Returns true if same component is allowed
    virtual bool            AllowSameComponent() const override { return true; }

                            /// Gets full list of property info including script properties.
    virtual void            GetPropertyInfoList(Array<PropertyInfo> &propertyInfoList) const override;

                            /// Deserialize from JSON value.
    virtual void            Deserialize(const Json::Value &in) override;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called once when game started.
                            /// When game already started, called immediately after spawned
    virtual void            Start() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;
                            /// Called on game world late-update, variable timestep.
    virtual void            LateUpdate() override;
                            /// Called on physics update, fixed timestep.
    virtual void            FixedUpdate(float timeStep) override;
                            /// Called on physics late-update, fixed timestep.
    virtual void            FixedLateUpdate(float timeStep) override;

    virtual void            OnPointerEnter();
    virtual void            OnPointerExit();
    virtual void            OnPointerOver();
    virtual void            OnPointerDown();
    virtual void            OnPointerUp();
    virtual void            OnPointerDrag();
    virtual void            OnPointerClick();

    virtual void            OnCollisionEnter(const Collision &collision);
    virtual void            OnCollisionExit(const Collision &collision);
    virtual void            OnCollisionStay(const Collision &collision);

    virtual void            OnSensorEnter(const Entity *entity);
    virtual void            OnSensorExit(const Entity *entity);
    virtual void            OnSensorStay(const Entity *entity);

    virtual void            OnParticleCollision(const Entity *entity);
    
    void                    OnApplicationTerminate();
    void                    OnApplicationPause(bool pause);

    Guid                    GetScriptGuid() const;
    void                    SetScriptGuid(const Guid &guid);

    bool                    HasError() const { return hasError;  }

    const char *            GetSandboxName() const { return sandboxName.c_str(); }

    template <typename... Args>
    void                    CallFunc(const char *funcName, Args&&... args);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    ChangeScript(const Guid &scriptGuid);
    void                    InitScriptFields();
    void                    SetScriptProperties();
    LuaCpp::Selector        CacheFunction(const char *funcname);
    void                    UpdateFunctionMap();
    void                    ClearFunctionMap();
    void                    ScriptReloaded();
    void                    SetOwnerValues();

    Guid                    scriptGuid;
    ScriptAsset *           scriptAsset;
    LuaCpp::State *         state;
    Str                     sandboxName;
    LuaCpp::Selector        sandbox;
    bool                    hasError;

    Array<PropertyInfo>     fieldInfos;
    HashMap<Str, Variant>   fieldValues;

    LuaCpp::Selector        awakeFunc;
    LuaCpp::Selector        startFunc;
    LuaCpp::Selector        updateFunc;
    LuaCpp::Selector        lateUpdateFunc;
    LuaCpp::Selector        fixedUpdateFunc;
    LuaCpp::Selector        fixedLateUpdateFunc;
    LuaCpp::Selector        onEnableFunc;
    LuaCpp::Selector        onDisableFunc;
    LuaCpp::Selector        onPointerEnterFunc;
    LuaCpp::Selector        onPointerExitFunc;
    LuaCpp::Selector        onPointerOverFunc;
    LuaCpp::Selector        onPointerDownFunc;
    LuaCpp::Selector        onPointerUpFunc;
    LuaCpp::Selector        onPointerDragFunc;
    LuaCpp::Selector        onPointerClickFunc;
    LuaCpp::Selector        onCollisionEnterFunc;
    LuaCpp::Selector        onCollisionExitFunc;
    LuaCpp::Selector        onCollisionStayFunc;
    LuaCpp::Selector        onSensorEnterFunc;
    LuaCpp::Selector        onSensorExitFunc;
    LuaCpp::Selector        onSensorStayFunc;
    LuaCpp::Selector        onParticleCollisionFunc;
    LuaCpp::Selector        onApplicationTerminateFunc;
    LuaCpp::Selector        onApplicationPauseFunc;
};

template <typename... Args>
BE_INLINE void ComScript::CallFunc(const char *funcName, Args&&... args) {
    auto func = sandbox[funcName];
    if (func.IsFunction()) {
        func(std::forward<Args>(args)...);
    }
}

BE_NAMESPACE_END
