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

void ComScript::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("script", "Script", Guid, GetScriptGuid, SetScriptGuid, Guid::zero, "", PropertyInfo::Editor).SetMetaObject(&ScriptAsset::metaObject);
}

ComScript::ComScript() {
    scriptAsset = nullptr;
}

ComScript::~ComScript() {
    Purge(false);
}

void ComScript::GetPropertyInfoList(Array<PropertyInfo> &propInfos) const {
    Component::GetPropertyInfoList(propInfos);

    for (int index = 0; index < fieldInfos.Count(); index++) {
        propInfos.Append(fieldInfos[index]);
    }
}

void ComScript::Purge(bool chainPurge) {
    LuaVM::State().SetToNil(sandbox.Name().c_str());

    fieldInfos.Clear();

    if (chainPurge) {
        Component::Purge();
    }
}

void ComScript::Init() {
    Component::Init();

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

void ComScript::InitScriptPropertyInfo(Json::Value &jsonComponent) {
    const Str scriptGuidString = jsonComponent.get("script", Guid::zero.ToString()).asCString();
    const Guid scriptGuid = Guid::FromString(scriptGuidString);

    // Sandbox name is same as GUID in string
    sandboxName = GetGuid().ToString();

    InitScriptPropertyInfoImpl(scriptGuid);
}

void ComScript::InitScriptPropertyInfoImpl(const Guid &scriptGuid) {
    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);

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

            if (!Str::Cmp(type, "int")) {
                int value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }

                    auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                        [pairPtr]() { return pairPtr->second.As<int>(); },
                        [pairPtr](int value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);
                    propInfo.SetRange(minimum, maximum, step);

                    fieldInfos.Append(propInfo);
                } else {
                    auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                        [pairPtr]() { return pairPtr->second.As<int>(); },
                        [pairPtr](int value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                    fieldInfos.Append(propInfo);
                }
            } else if (!Str::Cmp(type, "enum")) {
                int value = props["value"];
                const char *enumSequence = props["sequence"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                    [pairPtr]() { return pairPtr->second.As<int>(); },
                    [pairPtr](int value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);
                propInfo.SetEnumString(enumSequence);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "bool")) {
                bool value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<bool>::GetType(), new PropertyLambdaAccessorImpl<Class, bool>(
                    [pairPtr]() { return pairPtr->second.As<bool>(); },
                    [pairPtr](bool value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "float")) {
                float value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = props["minimum"];
                    float maximum = props["maximum"];
                    float step = props["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }

                    auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), new PropertyLambdaAccessorImpl<Class, float>(
                        [pairPtr]() { return pairPtr->second.As<float>(); },
                        [pairPtr](float value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);
                    propInfo.SetRange(minimum, maximum, step);

                    fieldInfos.Append(propInfo);
                } else {
                    auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), new PropertyLambdaAccessorImpl<Class, float>(
                        [pairPtr]() { return pairPtr->second.As<float>(); },
                        [pairPtr](float value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                    fieldInfos.Append(propInfo);
                }
            } else if (!Str::Cmp(type, "vec2")) {
                Vec2 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Vec2>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec2, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Vec2>(); },
                    [pairPtr](const Vec2 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "vec3")) {
                Vec3 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Vec3>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec3, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Vec3>(); },
                    [pairPtr](const Vec3 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "vec4")) {
                Vec4 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Vec4>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec4, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Vec4>(); },
                    [pairPtr](const Vec4 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "color3")) {
                Color3 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Color3>::GetType(), new PropertyLambdaAccessorImpl<Class, Color3, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Color3>(); },
                    [pairPtr](const Color3 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "color4")) {
                Color4 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Color4>::GetType(), new PropertyLambdaAccessorImpl<Class, Color4, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Color4>(); },
                    [pairPtr](const Color4 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "angles")) {
                Angles value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Angles>::GetType(), new PropertyLambdaAccessorImpl<Class, Angles, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Angles>(); },
                    [pairPtr](const Angles &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "quat")) {
                Quat value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Quat>::GetType(), new PropertyLambdaAccessorImpl<Class, Quat, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Quat>(); },
                    [pairPtr](const Quat &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "mat2")) {
                Mat2 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Mat2>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat2, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Mat2>(); },
                    [pairPtr](const Mat2 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "mat3")) {
                Mat3 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Mat3>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat3, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Mat3>(); },
                    [pairPtr](const Mat3 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "mat3x4")) {
                Mat3x4 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Mat3x4>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat3x4, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Mat3x4>(); },
                    [pairPtr](const Mat3x4 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "mat4")) {
                Mat4 value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Mat4>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat4, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Mat4>(); },
                    [pairPtr](const Mat4 &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "point")) {
                Point value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Point>::GetType(), new PropertyLambdaAccessorImpl<Class, Point, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Point>(); },
                    [pairPtr](const Point &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "rect")) {
                Rect value = props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Rect>::GetType(), new PropertyLambdaAccessorImpl<Class, Rect, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Rect>(); },
                    [pairPtr](const Rect &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "string")) {
                Str value = (const char *)props["value"];
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Str>::GetType(), new PropertyLambdaAccessorImpl<Class, Str, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Str>(); },
                    [pairPtr](const Str &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);

                fieldInfos.Append(propInfo);
            } else if (!Str::Cmp(type, "object")) {
                const char *classname = props["classname"];
                MetaObject *metaObject = Object::FindMetaObject(classname);
                Guid value = Guid::FromString((const char *)props["value"]);
                fieldValues.Set(name, value);

                auto pairPtr = fieldValues.Get(name);

                auto propInfo = PropertyInfo(name, label, VariantType<Guid>::GetType(), new PropertyLambdaAccessorImpl<Class, Guid, MixedPropertyTrait>(
                    [pairPtr]() { return pairPtr->second.As<Guid>(); },
                    [pairPtr](const Guid &value) { pairPtr->second = value; }), value, desc, PropertyInfo::Editor);
                propInfo.SetMetaObject(metaObject);

                fieldInfos.Append(propInfo);
            }
        };

        sandbox["property_names"].Enumerate(enumerator);
    }
}

void ComScript::ChangeScript(const Guid &scriptGuid) {
    // Disconnect with previously connected script asset
    if (scriptAsset) {
        scriptAsset->Disconnect(&Asset::SIG_Reloaded, this);
        scriptAsset = nullptr;
    }

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

static CVar lua_path(L"lua_path", L"", CVar::Archive, L"lua project path for debugging");

bool ComScript::LoadScriptWithSandbox(const char *filename, const char *sandboxName) {
    char *data;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    Str name;
#if 0
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
    LuaCpp::Selector properties = sandbox["properties"];

    for (int i = 0; i < fieldInfos.Count(); i++) {
        const PropertyInfo *propInfo = &fieldInfos[i];

        const char *name = propInfo->GetName();
        const Variant value = GetProperty(name);
        const Variant::Type type = propInfo->GetType();
        //assert(type == value.GetType());

        switch (type) {
        case Variant::IntType:
            properties[name]["value"] = value.As<int>();
            break;
        case Variant::Int64Type:
            properties[name]["value"] = value.As<int64_t>();
            break;
        case Variant::BoolType:
            properties[name]["value"] = value.As<bool>();
            break;
        case Variant::FloatType:
            properties[name]["value"] = value.As<float>();
            break;
        case Variant::DoubleType:
            properties[name]["value"] = value.As<double>();
            break;
        case Variant::Vec2Type:
            (Vec2 &)properties[name]["value"] = value.As<Vec2>();
            break;
        case Variant::Vec3Type:
            (Vec3 &)properties[name]["value"] = value.As<Vec3>();
            break;
        case Variant::Vec4Type:
            (Vec4 &)properties[name]["value"] = value.As<Vec4>();
            break;
        case Variant::Color3Type:
            (Color3 &)properties[name]["value"] = value.As<Color3>();
            break;
        case Variant::Color4Type:
            (Color4 &)properties[name]["value"] = value.As<Color4>();
            break;
        case Variant::AnglesType:
            (Angles &)properties[name]["value"] = value.As<Angles>();
            break;
        case Variant::QuatType:
            (Quat &)properties[name]["value"] = value.As<Quat>();
            break;
        case Variant::Mat2Type:
            (Mat2 &)properties[name]["value"] = value.As<Mat2>();
            break;
        case Variant::Mat3Type:
            (Mat3 &)properties[name]["value"] = value.As<Mat3>();
            break;
        case Variant::Mat3x4Type:
            (Mat3x4 &)properties[name]["value"] = value.As<Mat3x4>();
            break;
        case Variant::Mat4Type:
            (Mat4 &)properties[name]["value"] = value.As<Mat4>();
            break;
        case Variant::PointType:
            (Point &)properties[name]["value"] = value.As<Point>();
            break;
        case Variant::RectType:
            (Rect &)properties[name]["value"] = value.As<Rect>();
            break;
        case Variant::StrType:
            properties[name]["value"] = value.As<Str>().c_str();
            break;
        case Variant::GuidType: {
            Guid objectGuid = value.As<Guid>();
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
            break; 
        }
        default:
            assert(0);
            break;
        }
    }
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
    SetScriptGuid(GetProperty("script").As<Guid>());
}

Guid ComScript::GetScriptGuid() const {
    if (scriptAsset) {
        return scriptAsset->GetGuid();
    }
    return Guid();
}

void ComScript::SetScriptGuid(const Guid &guid) {
    if (!IsInitialized()) {
        ChangeScript(guid);
    } else {
        InitScriptPropertyInfoImpl(guid);

        ChangeScript(guid);

        // Update editor UI
        EmitSignal(&Serializable::SIG_PropertyInfoUpdated);
    }
}

BE_NAMESPACE_END
