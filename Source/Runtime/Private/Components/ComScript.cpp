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
#include "IO/FileSystem.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "Components/ComScript.h"
#include "Components/Transform/ComTransform.h"
#include "Components/Transform/ComRectTransform.h"
#include "Components/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Script", ComScript, ComLogic)
BEGIN_EVENTS(ComScript)
END_EVENTS

void ComScript::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("script", "Script", Guid, GetScriptGuid, SetScriptGuid, Guid::zero, 
        "", PropertyInfo::Flag::Editor).SetMetaObject(&ScriptResource::metaObject);
}

ComScript::ComScript() {
    updatable = true;
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

    started = false;
    hasError = false;

    fieldInfos.Clear();
    fieldGuids.Clear();

    if (chainPurge) {
        Component::Purge();
    }
}

void ComScript::OnActive() {
    if (IsInitialized()) {
        if (GetGameWorld()->IsGameStarted() && !started) {
            Start();
        }

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

    // Mark as initialized.
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
    owner["rect_transform"] = GetEntity()->GetRectTransform();
    owner["script"] = this;
}

void ComScript::GetPropertyInfoList(Array<PropertyInfo> &propInfos) const {
    Component::GetPropertyInfoList(propInfos);

    for (int index = 0; index < fieldInfos.Count(); index++) {
        propInfos.Append(fieldInfos[index]);
    }
}

void ComScript::Deserialize(const Json::Value &in) {
    // Get the script GUID in JSON value.
    const Str scriptGuidString = in.get("script", Guid::zero.ToString()).asCString();
    const Guid scriptGuid = Guid::FromString(scriptGuidString);

    state = &GetGameWorld()->GetLuaVM().State();

    ChangeScript(scriptGuid);

    deserializing = true;

    Serializable::Deserialize(in);

    deserializing = false;
}

void ComScript::ChangeScript(const Guid &scriptGuid) {
#if WITH_EDITOR
    // Disconnect with previously connected script asset.
    if (scriptAsset) {
        scriptAsset->Disconnect(&Asset::SIG_Reloaded, this);
        scriptAsset = nullptr;
    }
#endif

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

    // Sandbox name is same as component GUID in string.
    sandboxName = GetGuid().ToString();

    const Str scriptPath = resourceGuidMapper.Get(scriptGuid);
    char *text;
    size_t size = fileSystem.LoadFile(scriptPath, true, (void **)&text);
    if (!text) {
        sandboxName = "";
        BE_WARNLOG("ComScript::ChangeScript: Failed to load script '%s'\n", scriptPath.c_str());
        return;
    }

    if (!ParsePropertyNames(scriptPath, text, propertyNames)) {
        propertyNames.Clear();
    }

    // Load a script with sandboxed on current Lua state.
    if (!state->LoadBuffer(scriptPath.c_str(), text, size, sandboxName)) {
        hasError = true;
    }

    fileSystem.FreeFile(text);

    if (!hasError) {
        // Get the state of current loaded script.
        sandbox = (*state)[sandboxName];

        // Run this script.
        state->Run();

        // Check execute modes.
        if (sandbox["execute_in_edit_mode"].IsValid()) {
            executeInEditMode = sandbox["execute_in_edit_mode"];
        } else {
            executeInEditMode = false;
        }

        SetOwnerValues();

        UpdateFunctionMap();

        fieldInfos.Clear();
        fieldGuids.Clear();

        // Get the script property informations with this sandboxed script.
        if (sandbox["properties"].IsTable() && propertyNames.Count() > 0) {
            InitScriptFields();
        }
    }

#if WITH_EDITOR
    // Need to script asset to be reloaded in editor.
    Object *scriptObject = Asset::FindInstance(scriptGuid);
    if (scriptObject) {
        scriptAsset = scriptObject->Cast<Asset>();

        if (scriptAsset) {
            scriptAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComScript::ScriptReloaded, SignalObject::ConnectionType::Queued);
        }
    }
#endif
}

bool ComScript::ParsePropertyNames(const Str &textName, const Str &text, StrArray &propertyNames) {
    static const char *propertiesMarker = "--[properties]--";

    propertyNames.Clear();

    int index = text.Find(propertiesMarker);
    if (index == -1) {
        return false;
    }

    index += Str::Length(propertiesMarker);
    Lexer lexer(&text[index], text.Length() - index, textName);

    if (!lexer.ExpectTokenString("properties")) {
        return false;
    }
    if (!lexer.ExpectPunctuation(Lexer::PuncType::Assign)) {
        return false;
    }
    if (!lexer.ExpectPunctuation(Lexer::PuncType::BraceOpen)) {
        return false;
    }

    Str token;

    while (lexer.ReadToken(&token, true)) {
        if (token.IsEmpty()) {
            return false;
        }

        if (lexer.GetPunctuationType() == Lexer::PuncType::BraceClose) {
            break;
        }

        propertyNames.Append(token);

        if (!lexer.ExpectPunctuation(Lexer::PuncType::Assign)) {
            return false;
        }

        lexer.SkipBracedSection();

        lexer.ReadToken(&token, true);
        if (token == ",") {
            continue;
        }

        lexer.UnreadToken(&token);
    }

    return true;
}

void ComScript::InitScriptFields() {
    // Set zero values of object script properties.
    auto fieldGuidEnumerator = [this](const char *name) {
        auto prop = sandbox["properties"][name];
        if (!prop.IsValid()) {
            return;
        }

        const char *type = prop["type"];
        LuaCpp::Selector value = prop["value"];

        if (!Str::Cmp(type, "object")) {
            fieldGuids.Set(name, Guid::zero);
        }
    };

    // Create all the property info.
    auto fieldInfoEnumerator = [this](const char *name) {
        auto prop = sandbox["properties"][name];
        if (!prop.IsValid()) {
            return;
        }

        const char *label = prop["label"];
        const char *desc = prop["description"];
        const char *type = prop["type"];
        auto propValue = prop["value"];

        if (!label) {
            label = name;
        }

        if (!Str::Cmp(type, "int")) {
            if (prop["minimum"].LuaType() == LUA_TNUMBER && prop["maximum"].LuaType() == LUA_TNUMBER) {
                float minimum = prop["minimum"];
                float maximum = prop["maximum"];
                float step = prop["step"];

                Rangef range(minimum, maximum, step);
                if (step == 0.0f) {
                    range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                }

                auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), 
                    new PropertyLambdaAccessorImpl<Class, int>(
                        [propValue]() {
                            return (int)propValue;
                        },
                        [propValue, this](int value) {
                            propValue = value;
#if WITH_EDITOR
                            if (executeInEditMode) {
                                if (!deserializing) {
                                    OnValidate();
                                }
                            }
#endif
                        }
                    ), (int)propValue, desc, PropertyInfo::Flag::Editor);

                propInfo.SetRange(minimum, maximum, step);

                fieldInfos.Append(propInfo);
            } else {
                auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), 
                    new PropertyLambdaAccessorImpl<Class, int>(
                        [propValue]() {
                            return (int)propValue;
                        },
                        [propValue, this](int value) {
                            propValue = value;
#if WITH_EDITOR
                            if (executeInEditMode) {
                                if (!deserializing) {
                                    OnValidate();
                                }
                            }
#endif
                        }
                    ), (int)propValue, desc, PropertyInfo::Flag::Editor);

                fieldInfos.Append(propInfo);
            }
        } else if (!Str::Cmp(type, "enum")) {
            const char *enumSequence = prop["sequence"];

            auto propInfo = PropertyInfo(name, label, VariantType<int>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, int>(
                    [propValue]() {
                        return (int)propValue;
                    },
                    [propValue, this](int value) {
                        propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (int)propValue, desc, PropertyInfo::Flag::Editor);

                propInfo.SetEnumString(enumSequence);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "bool")) {
            auto propInfo = PropertyInfo(name, label, VariantType<bool>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, bool>(
                    [propValue]() {
                        return (bool)propValue;
                    },
                    [propValue, this](bool value) {
                        propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (bool)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "float")) {
            if (prop["minimum"].LuaType() == LUA_TNUMBER && prop["maximum"].LuaType() == LUA_TNUMBER) {
                float minimum = prop["minimum"];
                float maximum = prop["maximum"];
                float step = prop["step"];

                Rangef range(minimum, maximum, step);
                if (step == 0.0f) {
                    range.step = Math::Fabs((range.maxValue - range.minValue) / 100.0f);
                }

                auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), 
                    new PropertyLambdaAccessorImpl<Class, float>(
                        [propValue]() {
                            return (float)propValue;
                        },
                        [propValue, this](float value) {
                            propValue = value;
#if WITH_EDITOR
                            if (executeInEditMode) {
                                if (!deserializing) {
                                    OnValidate();
                                }
                            }
#endif
                        }
                    ), (float)propValue, desc, PropertyInfo::Flag::Editor);

                propInfo.SetRange(minimum, maximum, step);

                fieldInfos.Append(propInfo);
            } else {
                auto propInfo = PropertyInfo(name, label, VariantType<float>::GetType(), 
                    new PropertyLambdaAccessorImpl<Class, float>(
                        [propValue]() {
                            return (float)propValue;
                        },
                        [propValue, this](float value) {
                            propValue = value;
#if WITH_EDITOR
                            if (executeInEditMode) {
                                if (!deserializing) {
                                    OnValidate();
                                }
                            }
#endif
                        }
                    ), (float)propValue, desc, PropertyInfo::Flag::Editor);

                fieldInfos.Append(propInfo);
            }
        } else if (!Str::Cmp(type, "vec2")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec2>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Vec2, MixedPropertyTrait>(
                    [propValue]() {
                        return (Vec2 &)propValue;
                    },
                    [propValue, this](const Vec2 &value) { 
                        (Vec2 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Vec2 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "vec3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec3>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Vec3, MixedPropertyTrait>(
                    [propValue]() {
                        return (Vec3 &)propValue;
                    },
                    [propValue, this](const Vec3 &value) {
                        (Vec3 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Vec3 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "vec4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Vec4>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Vec4, MixedPropertyTrait>(
                    [propValue]() {
                        return (Vec4 &)propValue;
                    },
                    [propValue, this](const Vec4 &value) {
                        (Vec4 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Vec4 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "color3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Color3>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Color3, MixedPropertyTrait>(
                    [propValue]() {
                        return (Color3 &)propValue;
                    },
                    [propValue, this](const Color3 &value) {
                        (Color3 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Color3 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "color4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Color4>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Color4, MixedPropertyTrait>(
                    [propValue]() {
                        return (Color4 &)propValue;
                    },
                    [propValue, this](const Color4 &value) {
                        (Color4 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Color4 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "angles")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Angles>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Angles, MixedPropertyTrait>(
                    [propValue]() {
                        return (Angles &)propValue;
                    },
                    [propValue, this](const Angles &value) {
                        (Angles &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Angles &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "quat")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Quat>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Quat, MixedPropertyTrait>(
                    [propValue]() {
                        return (Quat &)propValue;
                    },
                    [propValue, this](const Quat &value) {
                        (Quat &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Quat &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat2")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat2>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Mat2, MixedPropertyTrait>(
                    [propValue]() {
                        return (Mat2 &)propValue;
                    },
                    [propValue, this](const Mat2 &value) {
                        (Mat2 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Mat2 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat3")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat3>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Mat3, MixedPropertyTrait>(
                    [propValue]() {
                        return (Mat3 &)propValue;
                    },
                    [propValue, this](const Mat3 &value) {
                        (Mat3 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Mat3 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat3x4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat3x4>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Mat3x4, MixedPropertyTrait>(
                    [propValue]() {
                        return (Mat3x4 &)propValue;
                    },
                    [propValue, this](const Mat3x4 &value) {
                        (Mat3x4 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Mat3x4 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "mat4")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Mat4>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Mat4, MixedPropertyTrait>(
                    [propValue]() {
                        return (Mat4 &)propValue;
                    },
                    [propValue, this](const Mat4 &value) {
                        (Mat4 &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Mat4 &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "point")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Point>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Point, MixedPropertyTrait>(
                    [propValue]() {
                        return (Point &)propValue;
                    },
                    [propValue, this](const Point &value) {
                        (Point &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Point &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "rect")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Rect>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Rect, MixedPropertyTrait>(
                    [propValue]() {
                        return (Rect &)propValue;
                    },
                    [propValue, this](const Rect &value) {
                        (Rect &)propValue = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (Rect &)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "string")) {
            auto propInfo = PropertyInfo(name, label, VariantType<Str>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Str, MixedPropertyTrait>(
                    [propValue]() {
                        return (const char *)propValue;
                    },
                    [propValue, this](const Str &value) {
                        propValue = value.c_str();
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), (const char *)propValue, desc, PropertyInfo::Flag::Editor);

            fieldInfos.Append(propInfo);
        } else if (!Str::Cmp(type, "object")) {
            auto pairPtr = fieldGuids.Get(name);
            int fieldIndex = fieldGuids.GetPairs().IndexOf(pairPtr);
            assert(fieldIndex >= 0);

            auto propInfo = PropertyInfo(name, label, VariantType<Guid>::GetType(), 
                new PropertyLambdaAccessorImpl<Class, Guid, MixedPropertyTrait>(
                    [fieldIndex, this]() {
                        auto pairPtr = fieldGuids.GetByIndex(fieldIndex);
                        return pairPtr->second.As<Guid>();
                    },
                    [fieldIndex, this](const Guid &value) {
                        auto pairPtr = fieldGuids.GetByIndex(fieldIndex);
                        pairPtr->second = value;
#if WITH_EDITOR
                        if (executeInEditMode) {
                            if (!deserializing) {
                                OnValidate();
                            }
                        }
#endif
                    }
                ), pairPtr->second.As<Guid>(), desc, PropertyInfo::Flag::Editor);

            const char *classname = prop["classname"];
            const MetaObject *metaObject = Object::FindMetaObject(classname);

            propInfo.SetMetaObject(metaObject);

            fieldInfos.Append(propInfo);
        }
    };

    for (int i = 0; i < propertyNames.Count(); i++) {
        fieldGuidEnumerator(propertyNames[i]);
        fieldInfoEnumerator(propertyNames[i]);
    }
}

void ComScript::ClearFunctionMap() {
    awakeFunc = LuaCpp::Selector();
    startFunc = LuaCpp::Selector();
    updateFunc = LuaCpp::Selector();
    lateUpdateFunc = LuaCpp::Selector();
    fixedUpdateFunc = LuaCpp::Selector();
    onValidateFunc = LuaCpp::Selector();
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
    onCharacterControllerHitFunc = LuaCpp::Selector();
    onParticleCollisionFunc = LuaCpp::Selector();
    onApplicationResizeFunc = LuaCpp::Selector();
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
    onValidateFunc = CacheFunction("on_validate");
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
    onCharacterControllerHitFunc = CacheFunction("on_character_controller_hit");
    onParticleCollisionFunc = CacheFunction("on_particle_collision");
    onApplicationResizeFunc = CacheFunction("on_application_resize");
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
        const Variant::Type::Enum type = propInfo->GetType();
        const Variant value = GetProperty(name);

        auto property = properties[name];

        switch (type) {
        case Variant::Type::Int:
            property["value"] = value.As<int>();
            break;
        case Variant::Type::Bool:
            property["value"] = value.As<bool>();
            break;
        case Variant::Type::Float:
            property["value"] = value.As<float>();
            break;
        case Variant::Type::Vec2:
            (Vec2 &)property["value"] = value.As<Vec2>();
            break;
        case Variant::Type::Vec3:
            (Vec3 &)property["value"] = value.As<Vec3>();
            break;
        case Variant::Type::Vec4:
            (Vec4 &)property["value"] = value.As<Vec4>();
            break;
        case Variant::Type::Color3:
            (Color3 &)property["value"] = value.As<Color3>();
            break;
        case Variant::Type::Color4:
            (Color4 &)property["value"] = value.As<Color4>();
            break;
        case Variant::Type::Angles:
            (Angles &)property["value"] = value.As<Angles>();
            break;
        case Variant::Type::Quat:
            (Quat &)property["value"] = value.As<Quat>();
            break;
        case Variant::Type::Mat2:
            (Mat2 &)property["value"] = value.As<Mat2>();
            break;
        case Variant::Type::Mat3:
            (Mat3 &)property["value"] = value.As<Mat3>();
            break;
        case Variant::Type::Mat3x4:
            (Mat3x4 &)property["value"] = value.As<Mat3x4>();
            break;
        case Variant::Type::Mat4:
            (Mat4 &)property["value"] = value.As<Mat4>();
            break;
        case Variant::Type::Point:
            (Point &)property["value"] = value.As<Point>();
            break;
        case Variant::Type::Size:
            (Size &)property["value"] = value.As<Size>();
            break;
        case Variant::Type::Rect:
            (Rect &)property["value"] = value.As<Rect>();
            break;
        case Variant::Type::Str:
            property["value"] = value.As<Str>().c_str();
            break;
        case Variant::Type::Guid: {
            Guid objectGuid = value.As<Guid>();
            Object *object = Object::FindInstance(objectGuid);
            if (object) {
                property["value"] = object;
            } else {
                if (propInfo->GetMetaObject()->IsTypeOf(Resource::metaObject)) {
                    if (!objectGuid.IsZero()) {
                        Asset *asset = (Asset *)Asset::CreateInstance(objectGuid); // FIXME: when to delete ?
                        asset->CreateResource(*propInfo->GetMetaObject());
                        property["value"] = asset;
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

    if (GetEntity()->IsActiveInHierarchy() && onEnableFunc.IsValid()) {
        onEnableFunc();
    }
}

void ComScript::Start() {
    if (startFunc.IsValid()) {
        startFunc();
    }
    started = true;
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

void ComScript::OnValidate() {
    if (onValidateFunc.IsValid()) {
        onValidateFunc();

        EmitSignal(&Serializable::SIG_PropertyInfoUpdated, 0);
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

void ComScript::OnCharacterControllerHit() {
    if (onCharacterControllerHitFunc.IsValid()) {
        onCharacterControllerHitFunc(entity);
    }
}

void ComScript::OnParticleCollision(const Entity *entity) {
    if (onParticleCollisionFunc.IsValid()) {
        onParticleCollisionFunc(entity);
    }
}

void ComScript::OnApplicationResize(int width, int height) {
    if (onApplicationResizeFunc.IsValid()) {
        onApplicationResizeFunc(width, height);
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

#if WITH_EDITOR
    // Update editor UI.
    EmitSignal(&Serializable::SIG_PropertyInfoUpdated, 1);
#endif
}

Guid ComScript::GetScriptGuid() const {
    return scriptGuid;
}

void ComScript::SetScriptGuid(const Guid &guid) {
    if (guid != scriptGuid) {
        ChangeScript(guid);
    }

#if WITH_EDITOR
    if (IsInitialized()) {
        // Update editor UI.
        EmitSignal(&Serializable::SIG_PropertyInfoUpdated, 1);
    }
#endif
}

BE_NAMESPACE_END
