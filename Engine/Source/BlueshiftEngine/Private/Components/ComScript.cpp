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
    PROPERTY_OBJECT("script", "Script", "", Guid::zero.ToString(), ScriptAsset::metaObject, PropertySpec::ReadWrite),
END_PROPERTIES

void ComScript::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Script", ScriptAsset, GetScript, SetScript, Guid::zero.ToString(), "", PropertySpec::ReadWrite);
}

ComScript::ComScript() {
    scriptAsset = nullptr;

    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComScript::PropertyChanged);
}

ComScript::~ComScript() {
    Purge(false);

    scriptPropertySpecs.DeleteContents(true);
}

void ComScript::GetPropertySpecList(Array<const PropertySpec *> &pspecs) const {
    Component::GetPropertySpecList(pspecs);

    pspecs.AppendList(scriptPropertySpecs);
}

void ComScript::InitPropertySpec(Json::Value &jsonComponent) {
    const Guid scriptGuid = Guid::ParseString(jsonComponent.get("script", Guid::zero.ToString()).asCString());

    InitPropertySpecImpl(scriptGuid);
}

void ComScript::InitPropertySpecImpl(const Guid &scriptGuid) {
    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);

    sandboxName = GetGuid().ToString();

    LoadScriptWithSandboxed(scriptPath, sandboxName);

    sandbox = LuaVM::State()[sandboxName];

    LuaVM::State().Run();

    scriptPropertySpecs.Clear();

    if (sandbox["properties"].IsTable() && sandbox["property_names"].IsTable()) {
        auto enumerator = [this](LuaCpp::Selector &selector) {
            const char *name = selector;
            auto prop = sandbox["properties"][name];
            const char *label = prop["label"];
            const char *desc = prop["description"];
            const char *type = prop["type"];

            if (!label) {
                label = name;
            }

            if (!Str::Cmp(type, "string")) {
                const char *value = prop["value"];
                scriptPropertySpecs.Append(new PROPERTY_STRING(name, label, desc, value, PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "enum")) {
                const char *sequence = prop["sequence"];
                Str value = Str((int)prop["value"]);
                scriptPropertySpecs.Append(new PROPERTY_ENUM(name, label, desc, sequence, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "float")) {
                Str value = Str((float)prop["value"]);
                if (prop["minimum"].LuaType() == LUA_TNUMBER && prop["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = prop["minimum"];
                    float maximum = prop["maximum"];
                    float step = prop["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }
                    scriptPropertySpecs.Append(new PROPERTY_RANGED_FLOAT(name, label, desc, range, value.c_str(), PropertySpec::ReadWrite));
                } else {
                    scriptPropertySpecs.Append(new PROPERTY_FLOAT(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
                }
            } else if (!Str::Cmp(type, "int")) {
                Str value = Str((int)prop["value"]);
                if (prop["minimum"].LuaType() == LUA_TNUMBER && prop["maximum"].LuaType() == LUA_TNUMBER) {
                    float minimum = prop["minimum"];
                    float maximum = prop["maximum"];
                    float step = prop["step"];

                    Rangef range(minimum, maximum, step);
                    if (step == 0.0f) {
                        range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                    }
                    scriptPropertySpecs.Append(new PROPERTY_RANGED_INT(name, label, desc, range, value.c_str(), PropertySpec::ReadWrite));
                } else {
                    scriptPropertySpecs.Append(new PROPERTY_INT(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
                }
            } else if (!Str::Cmp(type, "bool")) {
                Str value = Str((bool)prop["value"]);
                scriptPropertySpecs.Append(new PROPERTY_BOOL(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "point")) {
                Point point = prop["value"];
                Str value = point.ToString();
                scriptPropertySpecs.Append(new PROPERTY_POINT(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "rect")) {
                Rect rect = prop["value"];
                Str value = rect.ToString();
                scriptPropertySpecs.Append(new PROPERTY_RECT(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "vec2")) {
                Vec2 vec2 = prop["value"];
                Str value = vec2.ToString();
                scriptPropertySpecs.Append(new PROPERTY_VEC2(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "vec3")) {
                Vec3 vec3 = prop["value"];
                Str value = vec3.ToString();
                scriptPropertySpecs.Append(new PROPERTY_VEC3(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "vec4")) {
                Vec4 vec4 = prop["value"];
                Str value = vec4.ToString();
                scriptPropertySpecs.Append(new PROPERTY_VEC4(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "color3")) {
                Color3 color3 = prop["value"];
                Str value = color3.ToString();
                scriptPropertySpecs.Append(new PROPERTY_COLOR3(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "color4")) {
                Color4 color4 = prop["value"];
                Str value = color4.ToString();
                scriptPropertySpecs.Append(new PROPERTY_COLOR4(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "angles")) {
                Angles angles = prop["value"];
                Str value = angles.ToString();
                scriptPropertySpecs.Append(new PROPERTY_ANGLES(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "mat3")) {
                Mat3 mat3 = prop["value"];
                Str value = mat3.ToString();
                scriptPropertySpecs.Append(new PROPERTY_MAT3(name, label, desc, value.c_str(), PropertySpec::ReadWrite));
            } else if (!Str::Cmp(type, "object")) {
                const char *classname = prop["classname"];
                MetaObject *metaObject = Object::GetMetaObject(classname);
                if (metaObject) {
                    scriptPropertySpecs.Append(new PROPERTY_OBJECT(name, label, desc, Guid::zero.ToString(), *metaObject, PropertySpec::ReadWrite));
                }
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
    Purge();

    Component::Init();

    // Sandboxes with component GUID string
    sandboxName = GetGuid().ToString();

    ChangeScript(props->Get("script").As<Guid>());

    if (sandbox.IsValid()) {
        sandbox["owner"]["game_world"] = GetGameWorld();
        sandbox["owner"]["entity"] = GetEntity();
        sandbox["owner"]["name"] = GetEntity()->GetName();
        sandbox["owner"]["transform"] = GetEntity()->GetTransform();

        LuaVM::State().Run();
    }
}

void ComScript::ChangeScript(const Guid &scriptGuid) {
    // Disconnect from old script asset
    if (scriptAsset) {
        scriptAsset->Disconnect(&SIG_Reloaded, this);
    }

    LuaVM::State().SetToNil(sandboxName.c_str());

    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);
    if (scriptPath.IsEmpty()) {
        return;
    }

    if (!LoadScriptWithSandboxed(scriptPath, sandboxName)) {
        return;
    }

    // Need to script asset to be reloaded in Editor
    Object *scriptObject = ScriptAsset::FindInstance(scriptGuid);
    if (scriptObject) {
        scriptAsset = scriptObject->Cast<ScriptAsset>();
        
        if (scriptAsset) {
            scriptAsset->Connect(&SIG_Reloaded, this, (SignalCallback)&ComScript::ScriptReloaded, SignalObject::Queued);
        }
    }

    sandbox = LuaVM::State()[sandboxName];
}


static BE1::CVar lua_path(L"lua_path", L"", BE1::CVar::Archive, L"lua project path for debugging");

bool ComScript::LoadScriptWithSandboxed(const char *filename, const char *sandboxName) {
    char *data;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    // NOTE: absolute file path is needed for Lua debugging
    Str absFilename;
    
    char *path = tombs(lua_path.GetString());

    if (path[0]) {
        absFilename = path;
        absFilename.AppendPath(filename);
        absFilename.CleanPath();
    }
    else {
        absFilename = fileSystem.ToAbsolutePath(filename);
    }

    if (!LuaVM::State().LoadBuffer(absFilename.c_str(), data, size, sandboxName)) {
        fileSystem.FreeFile(data);
        return false;
    }

    fileSystem.FreeFile(data);

    return true;
}

void ComScript::SetScriptProperties() {
    LuaCpp::Selector properties = sandbox["properties"];

    for (int i = 0; i < scriptPropertySpecs.Count(); ++i) {
        const BE1::PropertySpec *spec = scriptPropertySpecs[i];

        const char *name = spec->GetName();
        const PropertySpec::Type type = spec->GetType();
        
        switch (type) {
        case PropertySpec::StringType:
            properties[name]["value"] = props->Get(name).As<Str>().c_str();
            break;
        case PropertySpec::FloatType:
            properties[name]["value"] = props->Get(name).As<float>();
            break;
        case PropertySpec::IntType:
        case PropertySpec::EnumType:
            properties[name]["value"] = props->Get(name).As<int>();
            break;
        case PropertySpec::BoolType:
            properties[name]["value"] = props->Get(name).As<bool>();
            break;
        case PropertySpec::PointType:
            (Point &)properties[name]["value"] = props->Get(name).As<Point>();
            break;
        case PropertySpec::RectType:
            (Rect &)properties[name]["value"] = props->Get(name).As<Rect>();
            break;
        case PropertySpec::Vec2Type:
            (Vec2 &)properties[name]["value"] = props->Get(name).As<Vec2>();
            break;
        case PropertySpec::Vec3Type:
        case PropertySpec::Color3Type:
            (Vec3 &)properties[name]["value"] = props->Get(name).As<Vec3>();
            break;
        case PropertySpec::Vec4Type:
        case PropertySpec::Color4Type:
            (Vec4 &)properties[name]["value"] = props->Get(name).As<Vec4>();
            break;
        case PropertySpec::AnglesType:
            (Angles &)properties[name]["value"] = props->Get(name).As<Angles>();
            break;
        case PropertySpec::Mat3Type:
            (Mat3 &)properties[name]["value"] = props->Get(name).As<Mat3>();
            break;
        case PropertySpec::ObjectType: {
            Guid objectGuid = props->Get(name).As<Guid>();
            Object *object = Object::FindInstance(objectGuid);
            if (object) {
                properties[name]["value"] = object;
            } else if (spec->GetMetaObject()->IsTypeOf(Asset::metaObject)) {
                object = spec->GetMetaObject()->CreateInstance(objectGuid); // FIXME: when to delete ?
                properties[name]["value"] = object;
            }
            
            if (!object) {
                BE_WARNLOG(L"not found object %hs\n", name);
            }
            break; }
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
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "script")) {
        SetScript(props->Get("script").As<Guid>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

const Guid ComScript::GetScript() const {
    if (scriptAsset) {
        return scriptAsset->GetGuid();
    }
    return Guid();
}

void ComScript::SetScript(const Guid &guid) {
    InitPropertySpecImpl(guid);

    ChangeScript(guid);

    EmitSignal(&SIG_UpdateUI);
}

BE_NAMESPACE_END
