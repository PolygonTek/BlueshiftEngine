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
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "File/FileSystem.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Script", ComScript, ComLogic)
BEGIN_EVENTS(ComScript)
END_EVENTS

void ComScript::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("script", "Script", Guid, GetScriptGuid, SetScriptGuid, Guid::zero, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&ScriptAsset::metaObject);
}

ComScript::ComScript() {
    state = nullptr;
    hasError = false;
    scriptAsset = nullptr;
}

ComScript::~ComScript() {
    Purge(false);
}

void ComScript::Purge(bool chainPurge) {
    if (!sandboxName.IsEmpty()) {
        state->SetToNil(sandboxName.c_str());
        sandboxName = "";
    }

    if (sandbox.IsValid()) {
        sandbox = LuaCpp::Selector();
    }

    hasError = false;

    fieldInfos.Clear();
    fieldValues.Clear();

    if (chainPurge) {
        Component::Purge();
    }
}

void ComScript::OnActive() {
    if (IsInitialized()) {
        if (onEnableFunc.IsValid()) {
            onEnableFunc();
        }
    }
}

void ComScript::OnInactive() {
    if (IsInitialized()) {
        if (onDisableFunc.IsValid()) {
            onDisableFunc();
        }
    }
}

void ComScript::Init() {
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComScript::SetOwnerValues() {
    if (!sandbox.IsValid()) {
        return;
    }

    auto owner = sandbox["owner"];
    owner["game_world"] = GetGameWorld();
    owner["entity"] = GetEntity();
    owner["name"] = GetEntity()->GetName().c_str();
    owner["transform"] = GetEntity()->GetTransform();
}

void ComScript::GetPropertyInfoList(Array<PropertyInfo> &propInfos) const {
    Component::GetPropertyInfoList(propInfos);

    for (int index = 0; index < fieldInfos.Count(); index++) {
        propInfos.Append(fieldInfos[index]);
    }
}

void ComScript::Deserialize(const Json::Value &in) {
    const Str scriptGuidString = in.get("script", Guid::zero.ToString()).asCString();
    const Guid scriptGuid = Guid::FromString(scriptGuidString);

    state = &GetGameWorld()->GetLuaVM().State();

    ChangeScript(scriptGuid);

    Serializable::Deserialize(in);
}

void ComScript::ChangeScript(const Guid &scriptGuid) {
    // Disconnect with previously connected script asset
    if (scriptAsset) {
        scriptAsset->Disconnect(&Asset::SIG_Reloaded, this);
        scriptAsset = nullptr;
    }

    if (!sandboxName.IsEmpty()) {
        state->SetToNil(sandboxName.c_str());
        //state->ForceGC();
    }

    if (sandbox.IsValid()) {
        sandbox = LuaCpp::Selector();
    }

    hasError = false;

    ClearFunctionMap();

    this->scriptGuid = scriptGuid;

    if (scriptGuid.IsZero()) {
        sandboxName = "";
        return;
    }

    // Sandbox name is same as component GUID in string
    sandboxName = GetGuid().ToString();

    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);
    char *data;
    size_t size = fileSystem.LoadFile(scriptPath, true, (void **)&data);
    if (!data) {
        sandboxName = "";
        BE_WARNLOG(L"ComScript::ChangeScript: Failed to load script '%hs'\n", scriptPath.c_str());
        return;
    }

    // Load a script with sandboxed on current Lua state
    if (!state->LoadBuffer(scriptPath.c_str(), data, size, sandboxName)) {
        hasError = true;
    }

    fileSystem.FreeFile(data);

    if (!hasError) {
        // Get the state of current loaded script
        sandbox = (*state)[sandboxName];

        // Run this script
        state->Run();

        SetOwnerValues();

        UpdateFunctionMap();

        fieldInfos.Clear();
        fieldValues.Clear();

        // Get the script property informations with this sandboxed script
        if (sandbox["properties"].IsTable() && sandbox["property_names"].IsTable()) {
            InitScriptFields();
        }
    }

    // Need to script asset to be reloaded in editor
    Object *scriptObject = ScriptAsset::FindInstance(scriptGuid);
    if (scriptObject) {
        scriptAsset = scriptObject->Cast<ScriptAsset>();

        if (scriptAsset) {
            scriptAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComScript::ScriptReloaded, SignalObject::Queued);
        }
    }
}

void ComScript::InitScriptFields() {
    // Get property default values to list up fieldValues
    auto fieldValueEnumerator = [this](LuaCpp::Selector &selector) {
        const char *name = selector;
        auto props = sandbox["properties"][name];
        const char *type = props["type"];
        LuaCpp::Selector value = props["value"];

        if (!Str::Cmp(type, "int")) {
            fieldValues.Set(name, (int)value);
        } else if (!Str::Cmp(type, "enum")) {
            fieldValues.Set(name, (int)value);
        } else if (!Str::Cmp(type, "bool")) {
            fieldValues.Set(name, (bool)value);
        } else if (!Str::Cmp(type, "float")) {
            fieldValues.Set(name, (float)value);
        } else if (!Str::Cmp(type, "vec2")) {
            fieldValues.Set(name, (Vec2 &)value);
        } else if (!Str::Cmp(type, "vec3")) {
            fieldValues.Set(name, (Vec3 &)value);
        } else if (!Str::Cmp(type, "vec4")) {
            fieldValues.Set(name, (Vec4 &)value);
        } else if (!Str::Cmp(type, "color3")) {
            fieldValues.Set(name, (Color3 &)value);
        } else if (!Str::Cmp(type, "color4")) {
            fieldValues.Set(name, (Color4 &)value);
        } else if (!Str::Cmp(type, "angles")) {
            fieldValues.Set(name, (Angles &)value);
        } else if (!Str::Cmp(type, "quat")) {
            fieldValues.Set(name, (Quat &)value);
        } else if (!Str::Cmp(type, "mat2")) {
            fieldValues.Set(name, (Mat2 &)value);
        } else if (!Str::Cmp(type, "mat3")) {
            fieldValues.Set(name, (Mat3 &)value);
        } else if (!Str::Cmp(type, "mat3x4")) {
            fieldValues.Set(name, (Mat3x4 &)value);
        } else if (!Str::Cmp(type, "mat4")) {
            fieldValues.Set(name, (Mat4 &)value);
        } else if (!Str::Cmp(type, "point")) {
            fieldValues.Set(name, (Point &)value);
        } else if (!Str::Cmp(type, "rect")) {
            fieldValues.Set(name, (Rect &)value);
        } else if (!Str::Cmp(type, "string")) {
            fieldValues.Set(name, Str((const char *)value));
        } else if (!Str::Cmp(type, "object")) {
            fieldValues.Set(name, Guid::FromString((const char *)value));
        } else {
            BE_WARNLOG(L"Invalid property type '%hs' for %hs\n", type, name);
        }
    };

    // Create all the property info
    auto fieldInfoEnumerator = [this](LuaCpp::Selector &selector) {
        const char *name = selector;
        auto props = sandbox["properties"][name];
        const char *label = props["label"];
        const char *desc = props["description"];
        const char *type = props["type"];

        if (!label) {
            label = name;
        }

        auto pairPtr = fieldValues.Get(name);

        if (!Str::Cmp(type, "int")) {
            if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                float minimum = props["minimum"];
                float maximum = props["maximum"];
                float step = props["step"];

                Rangef range(minimum, maximum, step);
                if (step == 0.0f) {
                    range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                }

                auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                    [pairPtr]() {
                    return pairPtr->second.As<int>();
                },
                    [pairPtr](int value) {
                    pairPtr->second = value;
                }), pairPtr->second.As<int>(), desc, PropertyInfo::EditorFlag);
                propInfo.SetRange(minimum, maximum, step);

                fieldInfos.Append(propInfo);
            } else {
                auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                    [pairPtr]() {
                    return pairPtr->second.As<int>();
                },
                    [pairPtr](int value) {
                    pairPtr->second = value;
                }), pairPtr->second.As<int>(), desc, PropertyInfo::EditorFlag);

                fieldInfos.Append(propInfo);
            }
        } else if (!Str::Cmp(type, "enum")) {
            const char *enumSequence = props["sequence"];

            auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), new PropertyLambdaAccessorImpl<Class, int>(
                [pairPtr]() {
                return pairPtr->second.As<int>();
            },
                [pairPtr](int value) {
                pairPtr->second = value;
            }), pairPtr->second.As<int>(), desc, PropertyInfo::EditorFlag);
            propInfo.SetEnumString(enumSequence);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "bool")) {
            auto propInfo = PropertyInfo(name, label, VariantType<bool>::GetType(), new PropertyLambdaAccessorImpl<Class, bool>(
                [pairPtr]() {
                return pairPtr->second.As<bool>();
            },
                [pairPtr](bool value) {
                pairPtr->second = value;
            }), pairPtr->second.As<bool>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "float")) {
            if (props["minimum"].LuaType() == LUA_TNUMBER && props["maximum"].LuaType() == LUA_TNUMBER) {
                float minimum = props["minimum"];
                float maximum = props["maximum"];
                float step = props["step"];

                Rangef range(minimum, maximum, step);
                if (step == 0.0f) {
                    range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                }

                auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), new PropertyLambdaAccessorImpl<Class, float>(
                    [pairPtr]() {
                    return pairPtr->second.As<float>();
                },
                    [pairPtr](float value) {
                    pairPtr->second = value;
                }), pairPtr->second.As<float>(), desc, PropertyInfo::EditorFlag);
                propInfo.SetRange(minimum, maximum, step);

                fieldInfos.Append(propInfo);
            } else {
                auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), new PropertyLambdaAccessorImpl<Class, float>(
                    [pairPtr]() {
                    return pairPtr->second.As<float>();
                },
                    [pairPtr](float value) {
                    pairPtr->second = value;
                }), pairPtr->second.As<float>(), desc, PropertyInfo::EditorFlag);

                fieldInfos.Append(propInfo);
            }
        } else if (!Str::Cmp(type, "vec2")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec2>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec2, MixedPropertyTrait>(
                [pairPtr]() { return pairPtr->second.As<Vec2>(); },
                [pairPtr](const Vec2 &value) { pairPtr->second = value; }), pairPtr->second.As<Vec2>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "vec3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec3>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec3, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Vec3>();
            },
                [pairPtr](const Vec3 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Vec3>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "vec4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec4>::GetType(), new PropertyLambdaAccessorImpl<Class, Vec4, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Vec4>();
            },
                [pairPtr](const Vec4 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Vec4>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "color3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Color3>::GetType(), new PropertyLambdaAccessorImpl<Class, Color3, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Color3>();
            },
                [pairPtr](const Color3 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Color3>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "color4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Color4>::GetType(), new PropertyLambdaAccessorImpl<Class, Color4, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Color4>();
            },
                [pairPtr](const Color4 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Color4>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "angles")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Angles>::GetType(), new PropertyLambdaAccessorImpl<Class, Angles, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Angles>();
            },
                [pairPtr](const Angles &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Angles>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "quat")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Quat>::GetType(), new PropertyLambdaAccessorImpl<Class, Quat, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Quat>();
            },
                [pairPtr](const Quat &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Quat>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat2")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat2>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat2, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Mat2>();
            },
                [pairPtr](const Mat2 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Mat2>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat3>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat3, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Mat3>();
            },
                [pairPtr](const Mat3 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Mat3>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat3x4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat3x4>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat3x4, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Mat3x4>();
            },
                [pairPtr](const Mat3x4 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Mat3x4>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat4>::GetType(), new PropertyLambdaAccessorImpl<Class, Mat4, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Mat4>();
            },
                [pairPtr](const Mat4 &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Mat4>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "point")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Point>::GetType(), new PropertyLambdaAccessorImpl<Class, Point, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Point>();
            },
                [pairPtr](const Point &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Point>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "rect")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Rect>::GetType(), new PropertyLambdaAccessorImpl<Class, Rect, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Rect>();
            },
                [pairPtr](const Rect &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Rect>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "string")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Str>::GetType(), new PropertyLambdaAccessorImpl<Class, Str, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Str>();
            },
                [pairPtr](const Str &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Str>(), desc, PropertyInfo::EditorFlag);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "object")) {
            const char *classname = props["classname"];
            const MetaObject *metaObject = Object::FindMetaObject(classname);

            auto propInfo = PropertyInfo(name, label, VariantType<Guid>::GetType(), new PropertyLambdaAccessorImpl<Class, Guid, MixedPropertyTrait>(
                [pairPtr]() {
                return pairPtr->second.As<Guid>();
            },
                [pairPtr](const Guid &value) {
                pairPtr->second = value;
            }), pairPtr->second.As<Guid>(), desc, PropertyInfo::EditorFlag);
            propInfo.SetMetaObject(metaObject);

            fieldInfos.Append(propInfo);
        }
    };

    sandbox["property_names"].Enumerate(fieldValueEnumerator);
    sandbox["property_names"].Enumerate(fieldInfoEnumerator);
}

void ComScript::ClearFunctionMap() {
    awakeFunc = LuaCpp::Selector();
    startFunc = LuaCpp::Selector();
    updateFunc = LuaCpp::Selector();
    lateUpdateFunc = LuaCpp::Selector();
    fixedUpdateFunc = LuaCpp::Selector();
    fixedLateUpdateFunc = LuaCpp::Selector();
    onEnableFunc = LuaCpp::Selector();
    onDisableFunc = LuaCpp::Selector();
    onPointerEnterFunc = LuaCpp::Selector();
    onPointerExitFunc = LuaCpp::Selector();
    onPointerOverFunc = LuaCpp::Selector();
    onPointerDownFunc = LuaCpp::Selector();
    onPointerUpFunc = LuaCpp::Selector();
    onPointerDragFunc = LuaCpp::Selector();
    onPointerClickFunc = LuaCpp::Selector();
    onCollisionEnterFunc = LuaCpp::Selector();
    onCollisionExitFunc = LuaCpp::Selector();
    onCollisionStayFunc = LuaCpp::Selector();
    onSensorEnterFunc = LuaCpp::Selector();
    onSensorExitFunc = LuaCpp::Selector();
    onSensorStayFunc = LuaCpp::Selector();
    onParticleCollisionFunc = LuaCpp::Selector();
    onApplicationTerminateFunc = LuaCpp::Selector();
    onApplicationPauseFunc = LuaCpp::Selector();
}

LuaCpp::Selector ComScript::CacheFunction(const char *funcname) {
    LuaCpp::Selector function = sandbox[funcname];
    if (function.IsFunction()) {
        return function;
    }
    return LuaCpp::Selector();
}

void ComScript::UpdateFunctionMap() {
    awakeFunc = CacheFunction("awake");
    startFunc = CacheFunction("start");
    updateFunc = CacheFunction("update");
    lateUpdateFunc = CacheFunction("late_update");
    fixedUpdateFunc = CacheFunction("fixed_update");
    fixedLateUpdateFunc = CacheFunction("fixed_late_update");
    onEnableFunc = CacheFunction("on_enable");
    onDisableFunc = CacheFunction("on_disable");
    onPointerEnterFunc = CacheFunction("on_pointer_enter");
    onPointerExitFunc = CacheFunction("on_pointer_exit");
    onPointerOverFunc = CacheFunction("on_pointer_over");
    onPointerDownFunc = CacheFunction("on_pointer_down");
    onPointerUpFunc = CacheFunction("on_pointer_up");
    onPointerDragFunc = CacheFunction("on_pointer_drag");
    onPointerClickFunc = CacheFunction("on_pointer_click");
    onCollisionEnterFunc = CacheFunction("on_collision_enter");
    onCollisionExitFunc = CacheFunction("on_collision_exit");
    onCollisionStayFunc = CacheFunction("on_collision_stay");
    onSensorEnterFunc = CacheFunction("on_sensor_enter");
    onSensorExitFunc = CacheFunction("on_sensor_exit");
    onSensorStayFunc = CacheFunction("on_sensor_stay");
    onParticleCollisionFunc = CacheFunction("on_particle_collision");
    onApplicationTerminateFunc = CacheFunction("on_application_terminate");
    onApplicationPauseFunc = CacheFunction("on_application_pause");
}

void ComScript::SetScriptProperties() {
    if (!sandbox.IsValid()) {
        return;
    }

    LuaCpp::Selector properties = sandbox["properties"];

    for (int i = 0; i < fieldInfos.Count(); i++) {
        const PropertyInfo *propInfo = &fieldInfos[i];
        const char *name = propInfo->GetName();
        const Variant::Type type = propInfo->GetType();
        const Variant value = GetProperty(name);

        auto property = properties[name];

        switch (type) {
        case Variant::IntType:
            property["value"] = value.As<int>();
            break;
        case Variant::Int64Type:
            property["value"] = value.As<int64_t>();
            break;
        case Variant::BoolType:
            property["value"] = value.As<bool>();
            break;
        case Variant::FloatType:
            property["value"] = value.As<float>();
            break;
        case Variant::DoubleType:
            property["value"] = value.As<double>();
            break;
        case Variant::Vec2Type:
            (Vec2 &)property["value"] = value.As<Vec2>();
            break;
        case Variant::Vec3Type:
            (Vec3 &)property["value"] = value.As<Vec3>();
            break;
        case Variant::Vec4Type:
            (Vec4 &)property["value"] = value.As<Vec4>();
            break;
        case Variant::Color3Type:
            (Color3 &)property["value"] = value.As<Color3>();
            break;
        case Variant::Color4Type:
            (Color4 &)property["value"] = value.As<Color4>();
            break;
        case Variant::AnglesType:
            (Angles &)property["value"] = value.As<Angles>();
            break;
        case Variant::QuatType:
            (Quat &)property["value"] = value.As<Quat>();
            break;
        case Variant::Mat2Type:
            (Mat2 &)property["value"] = value.As<Mat2>();
            break;
        case Variant::Mat3Type:
            (Mat3 &)property["value"] = value.As<Mat3>();
            break;
        case Variant::Mat3x4Type:
            (Mat3x4 &)property["value"] = value.As<Mat3x4>();
            break;
        case Variant::Mat4Type:
            (Mat4 &)property["value"] = value.As<Mat4>();
            break;
        case Variant::PointType:
            (Point &)property["value"] = value.As<Point>();
            break;
        case Variant::RectType:
            (Rect &)property["value"] = value.As<Rect>();
            break;
        case Variant::StrType:
            property["value"] = value.As<Str>().c_str();
            break;
        case Variant::GuidType: {
            Guid objectGuid = value.As<Guid>();
            Object *object = Object::FindInstance(objectGuid);
            if (object) {
                property["value"] = object;
            } else {
                if (propInfo->GetMetaObject()->IsTypeOf(Asset::metaObject)) {
                    if (!objectGuid.IsZero()) {
                        object = propInfo->GetMetaObject()->CreateInstance(objectGuid); // FIXME: when to delete ?
                        property["value"] = object;
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

    if (awakeFunc.IsValid()) {
        awakeFunc();
    }

    if (onEnableFunc.IsValid()) {
        onEnableFunc();
    }
}

void ComScript::Start() {
    if (startFunc.IsValid()) {
        startFunc();
    }
}

void ComScript::Update() {
    if (updateFunc.IsValid()) {
        updateFunc();
    }
}

void ComScript::LateUpdate() {
    if (lateUpdateFunc.IsValid()) {
        lateUpdateFunc();
    }
}

void ComScript::FixedUpdate(float timeStep) {
    if (fixedUpdateFunc.IsValid()) {
        fixedUpdateFunc(timeStep);
    }
}

void ComScript::FixedLateUpdate(float timeStep) {
    if (fixedLateUpdateFunc.IsValid()) {
        fixedLateUpdateFunc(timeStep);
    }
}

void ComScript::OnPointerEnter() {
    if (onPointerEnterFunc.IsValid()) {
        onPointerEnterFunc();
    }
}

void ComScript::OnPointerExit() {
    if (onPointerExitFunc.IsValid()) {
        onPointerExitFunc();
    }
}

void ComScript::OnPointerOver() {
    if (onPointerOverFunc.IsValid()) {
        onPointerOverFunc();
    }
}

void ComScript::OnPointerDown() {
    if (onPointerDownFunc.IsValid()) {
        onPointerDownFunc();
    }
}

void ComScript::OnPointerUp() {
    if (onPointerUpFunc.IsValid()) {
        onPointerUpFunc();
    }
}

void ComScript::OnPointerDrag() {
    if (onPointerDragFunc.IsValid()) {
        onPointerDragFunc();
    }
}

void ComScript::OnPointerClick() {
    if (onPointerClickFunc.IsValid()) {
        onPointerClickFunc();
    }
}

void ComScript::OnCollisionEnter(const Collision &collision) {
    if (onCollisionEnterFunc.IsValid()) {
        onCollisionEnterFunc(collision);
    }
}

void ComScript::OnCollisionExit(const Collision &collision) {
    if (onCollisionExitFunc.IsValid()) {
        onCollisionExitFunc(collision);
    }
}

void ComScript::OnCollisionStay(const Collision &collision) {
    if (onCollisionStayFunc.IsValid()) {
        onCollisionStayFunc(collision);
    }
}

void ComScript::OnSensorEnter(const Entity *entity) {
    if (onSensorEnterFunc.IsValid()) {
        onSensorEnterFunc(entity);
    }
}

void ComScript::OnSensorExit(const Entity *entity) {
    if (onSensorExitFunc.IsValid()) {
        onSensorExitFunc(entity);
    }
}

void ComScript::OnSensorStay(const Entity *entity) {
    if (onSensorStayFunc.IsValid()) {
        onSensorStayFunc(entity);
    }
}

void ComScript::OnParticleCollision(const Entity *entity) {
    if (onParticleCollisionFunc.IsValid()) {
        onParticleCollisionFunc(entity);
    }
}

void ComScript::OnApplicationTerminate() {
    if (onApplicationTerminateFunc.IsValid()) {
        onApplicationTerminateFunc(entity);
    }
}

void ComScript::OnApplicationPause(bool pause) {
    if (onApplicationPauseFunc.IsValid()) {
        onApplicationPauseFunc(pause);
    }
}

void ComScript::ScriptReloaded() {
    Json::Value value;
    Serializable::Serialize(value);

    Deserialize(value);

    // Update editor UI
    EmitSignal(&Serializable::SIG_PropertyInfoUpdated);
}

Guid ComScript::GetScriptGuid() const {
    return scriptGuid;
}

void ComScript::SetScriptGuid(const Guid &guid) {
    if (guid != scriptGuid) {
        ChangeScript(guid);
    }

    if (IsInitialized()) {
        // Update editor UI
        EmitSignal(&Serializable::SIG_PropertyInfoUpdated);
    }
}

BE_NAMESPACE_END
