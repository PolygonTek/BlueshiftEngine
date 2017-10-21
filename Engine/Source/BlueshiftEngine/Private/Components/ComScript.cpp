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
#include "Components/ComScript.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "File/FileSystem.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Script", ComScript, Component)
BEGIN_EVENTS(ComScript)
END_EVENTS
BEGIN_PROPERTIES(ComScript)
    PROPERTY_OBJECT("script", "Script", "", Guid::zero, ScriptAsset::metaObject, PropertyInfo::ReadWrite),
END_PROPERTIES

#ifdef NEW_PROPERTY_SYSTEM
void ComScript::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("Script", ObjectRef, GetScriptRef, SetScriptRef, ObjectRef(ScriptAsset::metaObject, Guid::zero), "", PropertyInfo::ReadWrite);
}
#endif

ComScript::ComScript() {
    scriptAsset = nullptr;

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComScript::PropertyChanged);
#endif
}

ComScript::~ComScript() {
    Purge(false);

#ifdef NEW_PROPERTY_SYSTEM
    fieldInfos.Clear();
#else
    fieldInfos.DeleteContents(true);
#endif
}

void ComScript::GetPropertyInfoList(Array<PropertyInfo> &propInfos) const {
    Component::GetPropertyInfoList(propInfos);

#ifndef NEW_PROPERTY_SYSTEM
    for (int index = 0; index < fieldInfos.Count(); index++) {
        propInfos.Append(*fieldInfos[index]);
    }
#endif
}

void ComScript::InitPropertyInfo(Json::Value &jsonComponent) {
    const Str scriptGuidString = jsonComponent.get("script", Guid::zero.ToString()).asCString();
    const Guid scriptGuid = Guid::FromString(scriptGuidString);

    InitPropertyInfoImpl(scriptGuid);
}

void ComScript::InitPropertyInfoImpl(const Guid &scriptGuid) {
    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);

    const Str sandboxName = GetGuid().ToString();
    
    // Load a script with sandboxed on current Lua state
    LoadScriptWithSandbox(scriptPath, sandboxName);

    // Get the state of current loaded script
    sandbox = LuaVM::State()[sandboxName];

    // Run this 
    LuaVM::State().Run();

    fieldInfos.Clear();
    
    fieldValues.Clear();

    // Get the script property informations with this sandboxed script
    if (sandbox["properties"].IsTable() && sandbox["property_names"].IsTable()) {
        auto enumerator = [this](LuaCpp::Selector &selector) {
            const char *name = selector;
            auto props = sandbox["properties"][name];
            const char *label = props["label"];
            const char *desc = props["description"];
            const char *type = props["type"];

            if (!label) {
                label = name;
            }

            if (!Str::Cmp(type, "string")) {
#ifdef NEW_PROPERTY_SYSTEM
                Str value = (const char *)props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Str>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Str>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Str value = (const char *)props["value"];
                fieldInfos.Append(new PROPERTY_STRING(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "enum")) {
#ifdef NEW_PROPERTY_SYSTEM
                int value = props["value"];
                const char *enumSequence = props["sequence"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyInfo::Enum(enumSequence), (size_t)(&fieldValues.Get(name)->second.As<int>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                const char *sequence = props["sequence"];
                Str value = Str((int)props["value"]);
                fieldInfos.Append(new PROPERTY_ENUM(name, label, desc, sequence, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "float")) {
#ifdef NEW_PROPERTY_SYSTEM
                float value = props["value"];
                fieldValues.Set(name, value);

                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }

                    auto propInfo = PropertyInfo(name, PropertyTypeID<float>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<float>()), value, desc, PropertyInfo::ReadWrite);
                    propInfo.SetRange(minimum, maximum, step);
                    fieldInfos.Append(propInfo);
                } else {
                    auto propInfo = PropertyInfo(name, PropertyTypeID<float>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<float>()), value, desc, PropertyInfo::ReadWrite);
                    fieldInfos.Append(propInfo);
                }
#else
                Str value = Str((float)props["value"]);
                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }
                    fieldInfos.Append(new PROPERTY_RANGED_FLOAT(name, label, desc, range, value.c_str(), PropertyInfo::ReadWrite));
                } else {
                    fieldInfos.Append(new PROPERTY_FLOAT(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
                }
#endif
            } else if (!Str::Cmp(type, "int")) {
#ifdef NEW_PROPERTY_SYSTEM
                int value = props["value"];
                fieldValues.Set(name, value);

                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }

                    auto propInfo = PropertyInfo(name, PropertyTypeID<int>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<int>()), value, desc, PropertyInfo::ReadWrite);
                    propInfo.SetRange(minimum, maximum, step);
                    fieldInfos.Append(propInfo);
                } else {
                    auto propInfo = PropertyInfo(name, PropertyTypeID<int>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<int>()), value, desc, PropertyInfo::ReadWrite);
                    fieldInfos.Append(propInfo);
                }
#else
                Str value = Str((int)props["value"]);
                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }
                    fieldInfos.Append(new PROPERTY_RANGED_INT(name, label, desc, range, value.c_str(), PropertyInfo::ReadWrite));
                } else {
                    fieldInfos.Append(new PROPERTY_INT(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
                }
#endif
            } else if (!Str::Cmp(type, "bool")) {
#ifdef NEW_PROPERTY_SYSTEM
                bool value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<bool>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<bool>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Str value = Str((bool)props["value"]);
                fieldInfos.Append(new PROPERTY_BOOL(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "point")) {
#ifdef NEW_PROPERTY_SYSTEM
                Point value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Point>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Point>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Point point = props["value"];
                Str value = point.ToString();
                fieldInfos.Append(new PROPERTY_POINT(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "rect")) {
#ifdef NEW_PROPERTY_SYSTEM
                Rect value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Rect>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Rect>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Rect rect = props["value"];
                Str value = rect.ToString();
                fieldInfos.Append(new PROPERTY_RECT(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "vec2")) {
#ifdef NEW_PROPERTY_SYSTEM
                Vec2 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Vec2>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Vec2>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Vec2 vec2 = props["value"];
                Str value = vec2.ToString();
                fieldInfos.Append(new PROPERTY_VEC2(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "vec3")) {
#ifdef NEW_PROPERTY_SYSTEM
                Vec3 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Vec3>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Vec3>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Vec3 vec3 = props["value"];
                Str value = vec3.ToString();
                fieldInfos.Append(new PROPERTY_VEC3(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "vec4")) {
#ifdef NEW_PROPERTY_SYSTEM
                Vec4 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Vec4>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Vec4>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Vec4 vec4 = props["value"];
                Str value = vec4.ToString();
                fieldInfos.Append(new PROPERTY_VEC4(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "color3")) {
#ifdef NEW_PROPERTY_SYSTEM
                Color3 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Color3>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Color3>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Color3 color3 = props["value"];
                Str value = color3.ToString();
                fieldInfos.Append(new PROPERTY_COLOR3(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "color4")) {
#ifdef NEW_PROPERTY_SYSTEM
                Color4 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Color4>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Color4>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Color4 color4 = props["value"];
                Str value = color4.ToString();
                fieldInfos.Append(new PROPERTY_COLOR4(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "angles")) {
#ifdef NEW_PROPERTY_SYSTEM
                Angles value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Angles>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Angles>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Angles angles = props["value"];
                Str value = angles.ToString();
                fieldInfos.Append(new PROPERTY_ANGLES(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "mat3")) {
#ifdef NEW_PROPERTY_SYSTEM
                Mat3 value = props["value"];
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<Mat3>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<Mat3>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                Mat3 mat3 = props["value"];
                Str value = mat3.ToString();
                fieldInfos.Append(new PROPERTY_MAT3(name, label, desc, value.c_str(), PropertyInfo::ReadWrite));
#endif
            } else if (!Str::Cmp(type, "object")) {
#ifdef NEW_PROPERTY_SYSTEM
                const char *classname = props["classname"];
                MetaObject *metaObject = Object::GetMetaObject(classname);
                ObjectRef value = ObjectRef(*metaObject, Guid::FromString((const char *)props["value"]));
                fieldValues.Set(name, value);

                auto propInfo = PropertyInfo(name, PropertyTypeID<ObjectRef>::GetType(), (size_t)(&fieldValues.Get(name)->second.As<ObjectRef>()), value, desc, PropertyInfo::ReadWrite);
                fieldInfos.Append(propInfo);
#else
                const char *classname = props["classname"];
                MetaObject *metaObject = Object::GetMetaObject(classname);
                if (metaObject) {
                    fieldInfos.Append(new PROPERTY_OBJECT(name, label, desc, Guid::zero.ToString(), *metaObject, PropertyInfo::ReadWrite));
                }
#endif
            }
        };

        sandbox["property_names"].Enumerate(enumerator);
    }
}

void ComScript::Purge(bool chainPurge) {
    LuaVM::State().SetToNil(sandbox.Name().c_str());

    if (chainPurge) {
        Component::Purge();
    }
}

void ComScript::Init() {
    Component::Init();

#ifndef NEW_PROPERTY_SYSTEM
    Guid scriptGuid = props->Get("script").As<Guid>();

    ChangeScript(scriptGuid);
#endif

    sandboxName = GetGuid().ToString();

    if (sandbox.IsValid()) {
        auto &owner = sandbox["owner"];

        owner["game_world"] = GetGameWorld();
        owner["entity"] = GetEntity();
        owner["name"] = GetEntity()->GetName().c_str();
        owner["transform"] = GetEntity()->GetTransform();

        LuaVM::State().Run();
    }

    // Mark as initialized
    SetInitialized(true);
}

void ComScript::ChangeScript(const Guid &scriptGuid) {
    // Disconnect with previously connected script asset
    if (scriptAsset) {
        scriptAsset->Disconnect(&Asset::SIG_Reloaded, this);
        scriptAsset = nullptr;
    }

    // Sandboxes with component GUID string
    const Str sandboxName = GetGuid().ToString();

    LuaVM::State().SetToNil(sandboxName.c_str());

    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);
    if (scriptPath.IsEmpty()) {
        return;
    }

    if (!LoadScriptWithSandbox(scriptPath, sandboxName)) {
        return;
    }

    // Need to script asset to be reloaded in editor
    Object *scriptObject = ScriptAsset::FindInstance(scriptGuid);
    if (scriptObject) {
        scriptAsset = scriptObject->Cast<ScriptAsset>();
        
        if (scriptAsset) {
            scriptAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComScript::ScriptReloaded, SignalObject::Queued);
        }
    }

    sandbox = LuaVM::State()[sandboxName];
}

static BE1::CVar lua_path(L"lua_path", L"", BE1::CVar::Archive, L"lua project path for debugging");

bool ComScript::LoadScriptWithSandbox(const char *filename, const char *sandboxName) {
    char *data;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    Str name;
#if 1
    // NOTE: absolute file path is needed for Lua debugging
    char *path = tombs(lua_path.GetString());

    if (path[0]) {
        name = path;
        name.AppendPath(filename);
        name.CleanPath();
    } else {
        name = fileSystem.ToAbsolutePath(filename);
    }
#else
    name = filename;
#endif

    if (!LuaVM::State().LoadBuffer(name.c_str(), data, size, sandboxName)) {
        fileSystem.FreeFile(data);
        return false;
    }

    fileSystem.FreeFile(data);

    return true;
}

void ComScript::SetScriptProperties() {
#ifndef NEW_PROPERTY_SYSTEM
    LuaCpp::Selector properties = sandbox["properties"];

    for (int i = 0; i < fieldInfos.Count(); ++i) {
        const BE1::PropertyInfo *propInfo = fieldInfos[i];

        const char *name = propInfo->GetName();
        const PropertyInfo::Type type = propInfo->GetType();
        
        switch (type) {
        case PropertyInfo::StringType:
            properties[name]["value"] = props->Get(name).As<Str>().c_str();
            break;
        case PropertyInfo::FloatType:
            properties[name]["value"] = props->Get(name).As<float>();
            break;
        case PropertyInfo::IntType:
        case PropertyInfo::EnumType:
            properties[name]["value"] = props->Get(name).As<int>();
            break;
        case PropertyInfo::BoolType:
            properties[name]["value"] = props->Get(name).As<bool>();
            break;
        case PropertyInfo::PointType:
            (Point &)properties[name]["value"] = props->Get(name).As<Point>();
            break;
        case PropertyInfo::RectType:
            (Rect &)properties[name]["value"] = props->Get(name).As<Rect>();
            break;
        case PropertyInfo::Vec2Type:
            (Vec2 &)properties[name]["value"] = props->Get(name).As<Vec2>();
            break;
        case PropertyInfo::Vec3Type:
            (Vec3 &)properties[name]["value"] = props->Get(name).As<Vec3>();
            break;
        case PropertyInfo::Vec4Type:
            (Vec4 &)properties[name]["value"] = props->Get(name).As<Vec4>();
            break;
        case PropertyInfo::Color3Type:
            (Color3 &)properties[name]["value"] = props->Get(name).As<Color3>();
            break;
        case PropertyInfo::Color4Type:
            (Color4 &)properties[name]["value"] = props->Get(name).As<Color4>();
            break;
        case PropertyInfo::AnglesType:
            (Angles &)properties[name]["value"] = props->Get(name).As<Angles>();
            break;
        case PropertyInfo::Mat3Type:
            (Mat3 &)properties[name]["value"] = props->Get(name).As<Mat3>();
            break;
        case PropertyInfo::ObjectType: {
            Guid objectGuid = props->Get(name).As<Guid>();
            Object *object = Object::FindInstance(objectGuid);
            if (object) {
                properties[name]["value"] = object;
            } else {
                if (propInfo->GetMetaObject()->IsTypeOf(Asset::metaObject)) {
                    if (!objectGuid.IsZero()) {
                        object = propInfo->GetMetaObject()->CreateInstance(objectGuid); // FIXME: when to delete ?
                        properties[name]["value"] = object;
                    }
                }
            }
            break; }
        default:
            assert(0);
            break;
        }
    }
#endif
}

void ComScript::Awake() {
    SetScriptProperties();

    CallFunc("awake");
}

void ComScript::Start() {
    CallFunc("start");
}

void ComScript::Update() {
    CallFunc("update");
}

void ComScript::LateUpdate() {
    CallFunc("late_update");
}

void ComScript::OnPointerEnter() {
    CallFunc("on_pointer_enter");
}

void ComScript::OnPointerExit() {
    CallFunc("on_pointer_exit");
}

void ComScript::OnPointerOver() {
    CallFunc("on_pointer_over");
}

void ComScript::OnPointerDown() {
    CallFunc("on_pointer_down");
}

void ComScript::OnPointerUp() {
    CallFunc("on_pointer_up");
}

void ComScript::OnPointerDrag() {
    CallFunc("on_pointer_drag");
}

void ComScript::OnCollisionEnter(const Collision &collision) {
    CallFunc("on_collision_enter", collision);
}

void ComScript::OnCollisionExit(const Collision &collision) {
    CallFunc("on_collision_exit", entity);
}

void ComScript::OnCollisionStay(const Collision &collision) {
    CallFunc("on_collision_stay", entity);
}

void ComScript::OnSensorEnter(const Entity *entity) {
    CallFunc("on_sensor_enter", entity);
}

void ComScript::OnSensorExit(const Entity *entity) {
    CallFunc("on_sensor_exit", entity);
}

void ComScript::OnSensorStay(const Entity *entity) {
    CallFunc("on_sensor_stay", entity);
}

void ComScript::OnParticleCollision(const Entity *entity) {
    CallFunc("on_particle_collision", entity);
}

void ComScript::OnApplicationTerminate() {
    CallFunc("on_application_terminate");
}

void ComScript::OnApplicationPause(bool pause) {
    CallFunc("on_application_pause", pause);
}

void ComScript::ScriptReloaded() {
    SetScript(props->Get("script").As<Guid>());
}

void ComScript::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "script")) {
        SetScript(props->Get("script").As<Guid>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

Guid ComScript::GetScript() const {
    if (scriptAsset) {
        return scriptAsset->GetGuid();
    }
    return Guid();
}

void ComScript::SetScript(const Guid &guid) {
    InitPropertyInfoImpl(guid);

    ChangeScript(guid);

    EmitSignal(&Properties::SIG_UpdateUI);
}

ObjectRef ComScript::GetScriptRef() const {
    return ObjectRef(ScriptAsset::metaObject, scriptAsset ? scriptAsset->GetGuid() : Guid::zero);
}

void ComScript::SetScriptRef(const ObjectRef &scriptRef) {
    InitPropertyInfoImpl(scriptRef.objectGuid);

    ChangeScript(scriptRef.objectGuid);

    EmitSignal(&Properties::SIG_UpdateUI);
}

BE_NAMESPACE_END
