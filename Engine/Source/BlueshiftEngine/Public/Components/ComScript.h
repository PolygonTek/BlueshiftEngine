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

#include "Script/LuaVM.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class Collision;
class ScriptAsset;

class ComScript : public Component {
public:
    OBJECT_PROTOTYPE(ComScript);

    ComScript();
    virtual ~ComScript();

    void                    InitPropertySpec(Json::Value &jsonComponent);

    virtual void            GetPropertySpecList(Array<const PropertySpec *> &pspecs) const override;

    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Start() override;

    virtual void            Update() override;

    virtual void            LateUpdate() override;

    const char *            GetSandboxName() const { return sandboxName.c_str(); }

    template <typename... Args>
    void                    CallFunc(const char *funcName, Args&&... args);

    virtual void            OnPointerEnter();
    virtual void            OnPointerExit();
    virtual void            OnPointerOver();
    virtual void            OnPointerDown();
    virtual void            OnPointerUp();
    virtual void            OnPointerDrag();

    virtual void            OnCollisionEnter(const Collision &collision);
    virtual void            OnCollisionExit(const Collision &collision);
    virtual void            OnCollisionStay(const Collision &collision);

    virtual void            OnSensorEnter(const Entity *entity);
    virtual void            OnSensorExit(const Entity *entity);
    virtual void            OnSensorStay(const Entity *entity);

    virtual void            OnParticleCollision(const Entity *entity);
    
    void                    OnApplicationTerminate();
    void                    OnApplicationPause(bool pause);

    Guid                    GetScript() const;
    void                    SetScript(const Guid &guid);

protected:
    void                    InitPropertySpecImpl(const Guid &scriptGuid);
    bool                    LoadScriptWithSandboxed(const char *filename, const char *sandboxName);
    void                    SetScriptProperties();

    void                    ChangeScript(const Guid &scriptGuid);
    void                    ScriptReloaded();

    void                    PropertyChanged(const char *classname, const char *propName);

    ScriptAsset *           scriptAsset;
    Str                     sandboxName;
    LuaCpp::Selector        sandbox;

    Array<const PropertySpec *> scriptPropertySpecs;
};

template <typename... Args>
BE_INLINE void ComScript::CallFunc(const char *funcName, Args&&... args) {
    auto func = sandbox[funcName];
    if (func.IsFunction()) {
        func(std::forward<Args>(args)...);
    }
}

BE_NAMESPACE_END
