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
#include "Core/Serializable.h"
#include "Core/Object.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const SignalDef Serializable::SIG_PropertyChanged("Serializable::PropertyChanged", "si");
const SignalDef Serializable::SIG_PropertyArrayCountChanged("Serializable::PropertyArrayCountChanged", "s");
const SignalDef Serializable::SIG_PropertyInfoUpdated("Serializable::PropertyInfoUpdated", "i");

bool Serializable::GetPropertyInfo(int index, PropertyInfo &propertyInfo) const {
    Array<PropertyInfo> propertyInfoList;
    GetPropertyInfoList(propertyInfoList);

    if (index < 0 || index > propertyInfoList.Count() - 1) {
        return false;
    }

    propertyInfo = propertyInfoList[index];
    return true;
}

bool Serializable::GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const {
    Array<PropertyInfo> propertyInfoList;
    GetPropertyInfoList(propertyInfoList);

    for (int i = 0; i < propertyInfoList.Count(); i++) {
        if (!Str::Cmp(propertyInfoList[i].GetName(), name)) {
            propertyInfo = propertyInfoList[i];
            return true;
        }
    }

    return false;
}

void Serializable::Serialize(Json::Value &out, bool forCopying) const {
    Array<PropertyInfo> propertyInfoList;

    GetPropertyInfoList(propertyInfoList);

    for (int propertyIndex = 0; propertyIndex < propertyInfoList.Count(); propertyIndex++) {
        const PropertyInfo &propertyInfo = propertyInfoList[propertyIndex];

        if (propertyInfo.GetFlags() & PropertyInfo::Flag::SkipSerialization) {
            continue;
        }

        if (forCopying && (propertyInfo.GetFlags() & PropertyInfo::Flag::NonCopying)) {
            continue;
        }

        const char *name = propertyInfo.name.c_str();

        if (propertyInfo.GetFlags() & PropertyInfo::Flag::Array) {
            out[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < GetPropertyArrayCount(propertyIndex); elementIndex++) {
                Variant value;
                GetArrayProperty(propertyInfo, elementIndex, value);

                out[name][elementIndex] = value.ToJsonValue();
            }
        } else {
            Variant value;
            GetProperty(propertyInfo, value);

            out[name] = value.ToJsonValue();
        }
    }
}

void Serializable::Deserialize(const Json::Value &node) {
    Array<PropertyInfo> propertyInfoList;

    GetPropertyInfoList(propertyInfoList);

    for (int propertyIndex = 0; propertyIndex < propertyInfoList.Count(); propertyIndex++) {
        const PropertyInfo &propertyInfo = propertyInfoList[propertyIndex];

        if (propertyInfo.GetFlags() & PropertyInfo::Flag::ReadOnly) {
            continue;
        }

        const char *name = propertyInfo.name.c_str();
        const Variant::Type::Enum type = propertyInfo.GetType();
        const Variant defaultValue = propertyInfo.GetDefaultValue();

        if (propertyInfo.GetFlags() & PropertyInfo::Flag::Array) {
            const Json::Value subNode = node.get(name, Json::Value());

            SetPropertyArrayCount(propertyIndex, subNode.size());

            for (int elementIndex = 0; elementIndex < subNode.size(); elementIndex++) {
                switch (type) {
                case Variant::Type::Int: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<int>());
                    SetArrayProperty(propertyIndex, elementIndex, value.asInt());
                    break;
                }
                case Variant::Type::Int64: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<int64_t>());
                    SetArrayProperty(propertyIndex, elementIndex, value.asInt64());
                    break;
                }
                case Variant::Type::Bool: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<bool>());
                    SetArrayProperty(propertyIndex, elementIndex, value.asBool());
                    break; 
                }
                case Variant::Type::Float: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<float>());
                    SetArrayProperty(propertyIndex, elementIndex, value.asFloat());
                    break; 
                }
                case Variant::Type::Vec2: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec2>().ToString());
                    Vec2 v = value.type() == Json::stringValue ? Vec2::FromString(value.asCString()) : defaultValue.As<Vec2>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Vec3: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec3>().ToString());
                    Vec3 v = value.type() == Json::stringValue ? Vec3::FromString(value.asCString()) : defaultValue.As<Vec3>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Vec4: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec4>().ToString());
                    Vec4 v = value.type() == Json::stringValue ? Vec4::FromString(value.asCString()) : defaultValue.As<Vec4>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Color3: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Color3>().ToString());
                    Color3 v = value.type() == Json::stringValue ? Color3::FromString(value.asCString()) : defaultValue.As<Color3>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Color4: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Color4>().ToString());
                    Color4 v = value.type() == Json::stringValue ? Color4::FromString(value.asCString()) : defaultValue.As<Color4>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Mat2: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat2>().ToString());
                    Mat2 v = value.type() == Json::stringValue ? Mat2::FromString(value.asCString()) : defaultValue.As<Mat2>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Mat3: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat3>().ToString());
                    Mat3 v = value.type() == Json::stringValue ? Mat3::FromString(value.asCString()) : defaultValue.As<Mat3>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Mat3x4: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat3x4>().ToString());
                    Mat3x4 v = value.type() == Json::stringValue ? Mat3x4::FromString(value.asCString()) : defaultValue.As<Mat3x4>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Mat4: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat4>().ToString());
                    Mat4 v = value.type() == Json::stringValue ? Mat4::FromString(value.asCString()) : defaultValue.As<Mat4>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Angles: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Angles>().ToString());
                    Angles v = value.type() == Json::stringValue ? Angles::FromString(value.asCString()) : defaultValue.As<Angles>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break; 
                }
                case Variant::Type::Quat: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Quat>().ToString());
                    Quat v = value.type() == Json::stringValue ? Quat::FromString(value.asCString()) : defaultValue.As<Quat>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Point: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Point>().ToString());
                    Point v = value.type() == Json::stringValue ? Point::FromString(value.asCString()) : defaultValue.As<Point>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Rect: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Rect>().ToString());
                    Rect v = value.type() == Json::stringValue ? Rect::FromString(value.asCString()) : defaultValue.As<Rect>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Guid: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Guid>().ToString());
                    Guid v = value.type() == Json::stringValue ? Guid::FromString(value.asCString()) : defaultValue.As<Guid>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                case Variant::Type::Str: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Str>().c_str());
                    Str v = value.type() == Json::stringValue ? Str(value.asCString()) : defaultValue.As<Str>();
                    SetArrayProperty(propertyIndex, elementIndex, v);
                    break;
                }
                default:
                    assert(0);
                    break;
                }
            }
        } else {
            const Json::Value value = node.get(name, defaultValue.ToJsonValue());

            switch (type) {
            case Variant::Type::Int:
                SetProperty(propertyIndex, value.asInt());
                break;
            case Variant::Type::Int64:
                SetProperty(propertyIndex, value.asInt64());
                break;
            case Variant::Type::Bool:
                SetProperty(propertyIndex, value.asBool());
                break;
            case Variant::Type::Float:
                SetProperty(propertyIndex, value.asFloat());
                break;
            case Variant::Type::Vec2: {
                Vec2 v = value.type() == Json::stringValue ? Vec2::FromString(value.asCString()) : defaultValue.As<Vec2>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Vec3: {
                Vec3 v = value.type() == Json::stringValue ? Vec3::FromString(value.asCString()) : defaultValue.As<Vec3>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Vec4: {
                Vec4 v = value.type() == Json::stringValue ? Vec4::FromString(value.asCString()) : defaultValue.As<Vec4>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Color3: {
                Color3 v = value.type() == Json::stringValue ? Color3::FromString(value.asCString()) : defaultValue.As<Color3>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Color4: {
                Color4 v = value.type() == Json::stringValue ? Color4::FromString(value.asCString()) : defaultValue.As<Color4>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Mat2: {
                Mat2 v = value.type() == Json::stringValue ? Mat2::FromString(value.asCString()) : defaultValue.As<Mat2>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Mat3: {
                Mat3 v = value.type() == Json::stringValue ? Mat3::FromString(value.asCString()) : defaultValue.As<Mat3>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Mat3x4: {
                Mat3x4 v = value.type() == Json::stringValue ? Mat3x4::FromString(value.asCString()) : defaultValue.As<Mat3x4>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Mat4: {
                Mat4 v = value.type() == Json::stringValue ? Mat4::FromString(value.asCString()) : defaultValue.As<Mat4>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Angles: {
                Angles v = value.type() == Json::stringValue ? Angles::FromString(value.asCString()) : defaultValue.As<Angles>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Quat: {
                Quat v = value.type() == Json::stringValue ? Quat::FromString(value.asCString()) : defaultValue.As<Quat>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Point: {
                Point v = value.type() == Json::stringValue ? Point::FromString(value.asCString()) : defaultValue.As<Point>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Rect: {
                Rect v = value.type() == Json::stringValue ? Rect::FromString(value.asCString()) : defaultValue.As<Rect>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Guid: {
                Guid v = value.type() == Json::stringValue ? Guid::FromString(value.asCString()) : defaultValue.As<Guid>();
                SetProperty(propertyIndex, v);
                break;
            }
            case Variant::Type::Str: {
                Str v = value.type() == Json::stringValue ? Str(value.asCString()) : defaultValue.As<Str>();
                SetProperty(propertyIndex, v);
                break;
            }
            default:
                assert(0);
                break;
            }
        }
    }
}

Variant Serializable::GetPropertyDefault(const char *name) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::GetPropertyDefault: invalid property name '%s'\n", name);
        return out;
    }

    out = propertyInfo.GetDefaultValue();
    return out;
}

Variant Serializable::GetPropertyDefault(int index) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::GetPropertyDefault: invalid property index %i\n", index);
        return out;
    }

    out = propertyInfo.GetDefaultValue();
    return out;
}

Variant Serializable::GetProperty(int index) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::GetProperty: invalid property index %i\n", index);
        return out;
    }

    GetProperty(propertyInfo, out);
    return out;
}

Variant Serializable::GetProperty(const char *name) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::GetProperty: invalid property name '%s'\n", name);
        return out;
    }

    GetProperty(propertyInfo, out);
    return out;
}

void Serializable::GetProperty(const PropertyInfo &propertyInfo, Variant &out) const {
    if (propertyInfo.accessor) {
        propertyInfo.accessor->Get(this, out);
        return;
    }

    const void *src = reinterpret_cast<const byte *>(this) + propertyInfo.offset;

    switch (propertyInfo.GetType()) {
    case Variant::Type::Int:
        out = *(reinterpret_cast<const int *>(src));
        break;
    case Variant::Type::Int64:
        out = *(reinterpret_cast<const int64_t *>(src));
        break;
    case Variant::Type::Bool:
        out = *(reinterpret_cast<const bool *>(src));
        break;
    case Variant::Type::Float:
        out = *(reinterpret_cast<const float *>(src));
        break;
    case Variant::Type::Double:
        out = *(reinterpret_cast<const double *>(src));
        break;
    case Variant::Type::Vec2:
        out = *(reinterpret_cast<const Vec2 *>(src));
        break;
    case Variant::Type::Vec3:
        out = *(reinterpret_cast<const Vec3 *>(src));
        break;
    case Variant::Type::Vec4:
        out = *(reinterpret_cast<const Vec4 *>(src));
        break;
    case Variant::Type::Color3:
        out = *(reinterpret_cast<const Color3 *>(src));
        break;
    case Variant::Type::Color4:
        out = *(reinterpret_cast<const Color4 *>(src));
        break;
    case Variant::Type::Angles:
        out = *(reinterpret_cast<const Angles *>(src));
        break;
    case Variant::Type::Quat:
        out = *(reinterpret_cast<const Quat *>(src));
        break;
    case Variant::Type::Mat2:
        out = *(reinterpret_cast<const Mat2 *>(src));
        break;
    case Variant::Type::Mat3:
        out = *(reinterpret_cast<const Mat3 *>(src));
        break;
    case Variant::Type::Mat3x4:
        out = *(reinterpret_cast<const Mat3x4 *>(src));
        break;
    case Variant::Type::Mat4:
        out = *(reinterpret_cast<const Mat4 *>(src));
        break;
    case Variant::Type::Point:
        out = *(reinterpret_cast<const Point *>(src));
        break;
    case Variant::Type::Rect:
        out = *(reinterpret_cast<const Rect *>(src));
        break;
    case Variant::Type::Guid:
        out = *(reinterpret_cast<const Guid *>(src));
        break;
    case Variant::Type::Str:
        out = *(reinterpret_cast<const Str *>(src));
        break;
    case Variant::Type::MinMaxCurve:
        out = *(reinterpret_cast<const MinMaxCurve *>(src));
        break;
    default:
        assert(0);
        break;
    }

    return;
}

Variant Serializable::GetArrayProperty(int index, int elementIndex) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::GetArrayProperty: invalid property index %i\n", index);
        return out;
    }

    GetArrayProperty(propertyInfo, elementIndex, out);
    return out;
}

Variant Serializable::GetArrayProperty(const char *name, int elementIndex) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::GetArrayProperty: invalid property name '%s'\n", name);
        return out;
    }

    GetArrayProperty(propertyInfo, elementIndex, out);
    return out;
}

void Serializable::GetArrayProperty(const PropertyInfo &propertyInfo, int elementIndex, Variant &out) const {
    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::Array)) {
        BE_WARNLOG("Serializable::GetArrayProperty: property '%s' is not array\n", propertyInfo.name.c_str());
        return;
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Get(this, elementIndex, out);
        return;
    }

    const void *src = reinterpret_cast<const byte *>(this) + propertyInfo.offset;

    switch (propertyInfo.GetType()) {
    case Variant::Type::Int:
        out = (*reinterpret_cast<const Array<int> *>(src))[elementIndex];
        break;
    case Variant::Type::Int64:
        out = (*reinterpret_cast<const Array<int64_t> *>(src))[elementIndex];
        break;
    case Variant::Type::Bool:
        out = (*reinterpret_cast<const Array<bool> *>(src))[elementIndex];
        break;
    case Variant::Type::Float:
        out = (*reinterpret_cast<const Array<float> *>(src))[elementIndex];
        break;
    case Variant::Type::Double:
        out = (*reinterpret_cast<const Array<double> *>(src))[elementIndex];
        break;
    case Variant::Type::Vec2:
        out = (*reinterpret_cast<const Array<Vec2> *>(src))[elementIndex];
        break;
    case Variant::Type::Vec3:
        out = (*reinterpret_cast<const Array<Vec3> *>(src))[elementIndex];
        break;
    case Variant::Type::Vec4:
        out = (*reinterpret_cast<const Array<Vec4> *>(src))[elementIndex];
        break;
    case Variant::Type::Color3:
        out = (*reinterpret_cast<const Array<Color3> *>(src))[elementIndex];
        break;
    case Variant::Type::Color4:
        out = (*reinterpret_cast<const Array<Color4> *>(src))[elementIndex];
        break;
    case Variant::Type::Angles:
        out = (*reinterpret_cast<const Array<Angles> *>(src))[elementIndex];
        break;
    case Variant::Type::Quat:
        out = (*reinterpret_cast<const Array<Quat> *>(src))[elementIndex];
        break;
    case Variant::Type::Mat2:
        out = (*reinterpret_cast<const Array<Mat2> *>(src))[elementIndex];
        break;
    case Variant::Type::Mat3:
        out = (*reinterpret_cast<const Array<Mat3> *>(src))[elementIndex];
        break;
    case Variant::Type::Mat3x4:
        out = (*reinterpret_cast<const Array<Mat3x4> *>(src))[elementIndex];
        break;
    case Variant::Type::Mat4:
        out = (*reinterpret_cast<const Array<Mat4> *>(src))[elementIndex];
        break;
    case Variant::Type::Point:
        out = (*reinterpret_cast<const Array<Point> *>(src))[elementIndex];
        break;
    case Variant::Type::Rect:
        out = (*reinterpret_cast<const Array<Rect> *>(src))[elementIndex];
        break;
    case Variant::Type::Guid:
        out = (*reinterpret_cast<const Array<Guid> *>(src))[elementIndex];
        break;
    case Variant::Type::Str:
        out = (*reinterpret_cast<const Array<Str> *>(src))[elementIndex];
        break;
    case Variant::Type::MinMaxCurve:
        out = (*reinterpret_cast<const Array<MinMaxCurve> *>(src))[elementIndex];
        break;
    default:
        assert(0);
        break;
    }
}

bool Serializable::SetProperty(const char *name, const Variant &value, bool forceWrite) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::SetProperty: invalid property name '%s'\n", name);
        return false;
    }

    return SetProperty(propertyInfo, value, forceWrite);
}

bool Serializable::SetProperty(int index, const Variant &value, bool forceWrite) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::SetProperty: invalid property index %i\n", index);
        return false;
    }

    return SetProperty(propertyInfo, value, forceWrite);
}

bool Serializable::SetProperty(const PropertyInfo &propertyInfo, const Variant &value, bool forceWrite) {
    // You can force to write a value even though a property has read only flag.
    if (!forceWrite && (propertyInfo.GetFlags() & PropertyInfo::Flag::ReadOnly)) {
        BE_WARNLOG("Serializable::SetProperty: property '%s' is readonly\n", propertyInfo.name.c_str());
        return false;
    }

    Variant newValue;
    float minValue;
    float maxValue;

    bool ranged = propertyInfo.IsRanged();

    if (ranged) {
        minValue = propertyInfo.GetMinValue();
        maxValue = propertyInfo.GetMaxValue();
    }

    switch (propertyInfo.GetType()) {
    case Variant::Type::Int: {
        int i = value.As<int>();
        if (ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newValue = i;
        break; 
    }
    case Variant::Type::Int64: {
        int64_t i = value.As<int64_t>();
        if (ranged) {
            Clamp(i, (int64_t)minValue, (int64_t)maxValue);
        }
        newValue = i;
        break;
    }
    case Variant::Type::Bool:
        newValue = value.As<bool>();
        break;
    case Variant::Type::Float: {
        float f = value.As<float>();
        if (ranged) {
            Clamp(f, minValue, maxValue);
        }
        newValue = f;
        break;
    }
    case Variant::Type::Vec2: {
        Vec2 vec2 = value.As<Vec2>();
        if (ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newValue = vec2;
        break; 
    }
    case Variant::Type::Vec3: {
        Vec3 vec3 = value.As<Vec3>();
        if (ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newValue = vec3;
        break; 
    }
    case Variant::Type::Vec4: {
        Vec4 vec4 = value.As<Vec4>();
        if (ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newValue = vec4;
        break; 
    }
    case Variant::Type::Color3: {
        Color3 color3 = value.As<Color3>();
        if (ranged) {
            Clamp(color3.r, minValue, maxValue);
            Clamp(color3.g, minValue, maxValue);
            Clamp(color3.b, minValue, maxValue);
        }
        newValue = color3;
        break; 
    }
    case Variant::Type::Color4: {
        Color4 color4 = value.As<Color4>();
        if (ranged) {
            Clamp(color4.r, minValue, maxValue);
            Clamp(color4.g, minValue, maxValue);
            Clamp(color4.b, minValue, maxValue);
            Clamp(color4.a, minValue, maxValue);
        }
        newValue = color4;
        break; 
    }
    case Variant::Type::Mat2:
        newValue = value.As<Mat2>();
        break;
    case Variant::Type::Mat3:
        newValue = value.As<Mat3>();
        break;
    case Variant::Type::Mat3x4:
        newValue = value.As<Mat3x4>();
        break;
    case Variant::Type::Mat4:
        newValue = value.As<Mat4>();
        break;
    case Variant::Type::Angles: {
        Angles angles = value.As<Angles>();
        if (ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newValue = angles;
        break;
    }
    case Variant::Type::Quat: {
        newValue = value.As<Quat>();
        break;
    }
    case Variant::Type::Point: {
        Point pt = value.As<Point>();
        if (ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newValue = pt;
        break;
    }
    case Variant::Type::Rect: {
        Rect rect = value.As<Rect>();
        if (ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newValue = rect;
        break;
    }
    case Variant::Type::Guid:
        newValue = value.As<Guid>();
        break;
    case Variant::Type::Str:
        newValue = value.As<Str>();
        break;
    case Variant::Type::MinMaxCurve:
        newValue = value.As<MinMaxCurve>();
        break;
    default:
        assert(0);
        break;
    }

    Variant oldValue;
    GetProperty(propertyInfo, oldValue);

    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::ForceToSet)) {
        if (newValue == oldValue) {
            return true;
        }
    }
    
    if (propertyInfo.accessor) {
        propertyInfo.accessor->Set(this, newValue);
    } else {
        void *dest = reinterpret_cast<byte *>(this) + propertyInfo.offset;

        switch (propertyInfo.GetType()) {
        case Variant::Type::Int:
            *(reinterpret_cast<int *>(dest)) = newValue.As<int>();
            break;
        case Variant::Type::Int64:
            *(reinterpret_cast<int64_t *>(dest)) = newValue.As<int64_t>();
            break;
        case Variant::Type::Bool:
            *(reinterpret_cast<bool *>(dest)) = newValue.As<bool>();
            break;
        case Variant::Type::Float:
            *(reinterpret_cast<float *>(dest)) = newValue.As<float>();
            break;
        case Variant::Type::Double:
            *(reinterpret_cast<double *>(dest)) = newValue.As<double>();
            break;
        case Variant::Type::Vec2:
            *(reinterpret_cast<Vec2 *>(dest)) = newValue.As<Vec2>();
            break;
        case Variant::Type::Vec3:
            *(reinterpret_cast<Vec3 *>(dest)) = newValue.As<Vec3>();
            break;
        case Variant::Type::Vec4:
            *(reinterpret_cast<Vec4 *>(dest)) = newValue.As<Vec4>();
            break;
        case Variant::Type::Color3:
            *(reinterpret_cast<Color3 *>(dest)) = newValue.As<Color3>();
            break;
        case Variant::Type::Color4:
            *(reinterpret_cast<Color4 *>(dest)) = newValue.As<Color4>();
            break;
        case Variant::Type::Mat2:
            *(reinterpret_cast<Mat2 *>(dest)) = newValue.As<Mat2>();
            break;
        case Variant::Type::Mat3:
            *(reinterpret_cast<Mat3 *>(dest)) = newValue.As<Mat3>();
            break;
        case Variant::Type::Mat3x4:
            *(reinterpret_cast<Mat3x4 *>(dest)) = newValue.As<Mat3x4>();
            break;
        case Variant::Type::Mat4:
            *(reinterpret_cast<Mat4 *>(dest)) = newValue.As<Mat4>();
            break;
        case Variant::Type::Angles:
            *(reinterpret_cast<Angles *>(dest)) = newValue.As<Angles>();
            break;
        case Variant::Type::Quat:
            *(reinterpret_cast<Quat *>(dest)) = newValue.As<Quat>();
            break;
        case Variant::Type::Point:
            *(reinterpret_cast<Point *>(dest)) = newValue.As<Point>();
            break;
        case Variant::Type::Rect:
            *(reinterpret_cast<Rect *>(dest)) = newValue.As<Rect>();
            break;
        case Variant::Type::Guid:
            *(reinterpret_cast<Guid *>(dest)) = newValue.As<Guid>();
            break;
        case Variant::Type::Str:
            *(reinterpret_cast<Str *>(dest)) = newValue.As<Str>();
            break;
        case Variant::Type::MinMaxCurve:
            *(reinterpret_cast<MinMaxCurve *>(dest)) = newValue.As<MinMaxCurve>();
            break;
        default:
            assert(0);
            break;
        }
    }

    EmitSignal(&Serializable::SIG_PropertyChanged, propertyInfo.name.c_str(), -1);
    return true;
}

bool Serializable::SetArrayProperty(const char *name, int elementIndex, const Variant &value, bool forceWrite) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::SetArrayProperty: invalid property name '%s'\n", name);
        return false;
    }

    return SetArrayProperty(propertyInfo, elementIndex, value, forceWrite);
}

bool Serializable::SetArrayProperty(int index, int elementIndex, const Variant &value, bool forceWrite) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::SetArrayProperty: invalid property index %i\n", index);
        return false;
    }

    return SetArrayProperty(propertyInfo, elementIndex, value, forceWrite);
}

bool Serializable::SetArrayProperty(const PropertyInfo &propertyInfo, int elementIndex, const Variant &value, bool forceWrite) {
    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::Array)) {
        BE_WARNLOG("Serializable::SetArrayProperty: property '%s' is not array\n", propertyInfo.name.c_str());
        return false;
    }

    if (!forceWrite && (propertyInfo.GetFlags() & PropertyInfo::Flag::ReadOnly)) {
        BE_WARNLOG("Serializable::SetArrayProperty: property '%s' is readonly\n", propertyInfo.name.c_str());
        return false;
    }

    Variant newValue;
    float minValue;
    float maxValue;

    bool ranged = propertyInfo.IsRanged();

    if (ranged) {
        minValue = propertyInfo.GetMinValue();
        maxValue = propertyInfo.GetMaxValue();
    }

    switch (propertyInfo.GetType()) {
    case Variant::Type::Int: {
        int i = value.As<int>();
        if (ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newValue = i;
        break; 
    }
    case Variant::Type::Int64: {
        int64_t i = value.As<int64_t>();
        if (ranged) {
            Clamp(i, (int64_t)minValue, (int64_t)maxValue);
        }
        newValue = i;
        break;
    }
    case Variant::Type::Bool:
        newValue = value.As<bool>();
        break;
    case Variant::Type::Float: {
        float f = value.As<float>();
        if (ranged) {
            Clamp(f, minValue, maxValue);
        }
        newValue = f;
        break;
    }
    case Variant::Type::Vec2: {
        Vec2 vec2 = value.As<Vec2>();
        if (ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newValue = vec2;
        break; 
    }
    case Variant::Type::Vec3: {
        Vec3 vec3 = value.As<Vec3>();
        if (ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newValue = vec3;
        break; 
    }
    case Variant::Type::Vec4: {
        Vec4 vec4 = value.As<Vec4>();
        if (ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newValue = vec4;
        break; 
    }
    case Variant::Type::Color3: {
        Color3 color3 = value.As<Color3>();
        if (ranged) {
            Clamp(color3.r, minValue, maxValue);
            Clamp(color3.g, minValue, maxValue);
            Clamp(color3.b, minValue, maxValue);
        }
        newValue = color3;
        break; 
    }
    case Variant::Type::Color4: {
        Color4 color4 = value.As<Color4>();
        if (ranged) {
            Clamp(color4.r, minValue, maxValue);
            Clamp(color4.g, minValue, maxValue);
            Clamp(color4.b, minValue, maxValue);
            Clamp(color4.a, minValue, maxValue);
        }
        newValue = color4;
        break; 
    }
    case Variant::Type::Mat2:
        newValue = value.As<Mat2>();
        break;
    case Variant::Type::Mat3:
        newValue = value.As<Mat3>();
        break;
    case Variant::Type::Mat3x4:
        newValue = value.As<Mat3x4>();
        break;
    case Variant::Type::Mat4:
        newValue = value.As<Mat4>();
        break;
    case Variant::Type::Angles: {
        Angles angles = value.As<Angles>();
        if (ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newValue = angles;
        break;
    }
    case Variant::Type::Point: {
        Point pt = value.As<Point>();
        if (ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newValue = pt;
        break;
    }
    case Variant::Type::Rect: {
        Rect rect = value.As<Rect>();
        if (ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newValue = rect;
        break;
    }
    case Variant::Type::Guid:
        newValue = value.As<Guid>();
        break;
    case Variant::Type::Str:
        newValue = value.As<Str>();
        break;
    case Variant::Type::MinMaxCurve:
        newValue = value.As<MinMaxCurve>();
        break;
    default:
        assert(0);
        break;
    }

    Variant oldValue;
    GetArrayProperty(propertyInfo, elementIndex, oldValue);

    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::ForceToSet)) {
        if (newValue == oldValue) {
            return true;
        }
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Set(this, elementIndex, newValue);
    } else {
        void *dest = reinterpret_cast<byte *>(this) + propertyInfo.offset;

        switch (propertyInfo.GetType()) {
        case Variant::Type::Int:
            (*reinterpret_cast<Array<int> *>(dest))[elementIndex] = newValue.As<int>();
            break;
        case Variant::Type::Int64:
            (*reinterpret_cast<Array<int64_t> *>(dest))[elementIndex] = newValue.As<int64_t>();
            break;
        case Variant::Type::Bool:
            (*reinterpret_cast<Array<bool> *>(dest))[elementIndex] = newValue.As<bool>();
            break;
        case Variant::Type::Float:
            (*reinterpret_cast<Array<float> *>(dest))[elementIndex] = newValue.As<float>();
            break;
        case Variant::Type::Double:
            (*reinterpret_cast<Array<double> *>(dest))[elementIndex] = newValue.As<double>();
            break;
        case Variant::Type::Vec2:
            (*reinterpret_cast<Array<Vec2> *>(dest))[elementIndex] = newValue.As<Vec2>();
            break;
        case Variant::Type::Vec3:
            (*reinterpret_cast<Array<Vec3> *>(dest))[elementIndex] = newValue.As<Vec3>();
            break;
        case Variant::Type::Vec4:
            (*reinterpret_cast<Array<Vec4> *>(dest))[elementIndex] = newValue.As<Vec4>();
            break;
        case Variant::Type::Color3:
            (*reinterpret_cast<Array<Color3> *>(dest))[elementIndex] = newValue.As<Color3>();
            break;
        case Variant::Type::Color4:
            (*reinterpret_cast<Array<Color4> *>(dest))[elementIndex] = newValue.As<Color4>();
            break;
        case Variant::Type::Mat2:
            (*reinterpret_cast<Array<Mat2> *>(dest))[elementIndex] = newValue.As<Mat2>();
            break;
        case Variant::Type::Mat3:
            (*reinterpret_cast<Array<Mat3> *>(dest))[elementIndex] = newValue.As<Mat3>();
            break;
        case Variant::Type::Mat3x4:
            (*reinterpret_cast<Array<Mat3x4> *>(dest))[elementIndex] = newValue.As<Mat3x4>();
            break;
        case Variant::Type::Mat4:
            (*reinterpret_cast<Array<Mat4> *>(dest))[elementIndex] = newValue.As<Mat4>();
            break;
        case Variant::Type::Angles:
            (*reinterpret_cast<Array<Angles> *>(dest))[elementIndex] = newValue.As<Angles>();
            break;
        case Variant::Type::Quat:
            (*reinterpret_cast<Array<Quat> *>(dest))[elementIndex] = newValue.As<Quat>();
            break;
        case Variant::Type::Point:
            (*reinterpret_cast<Array<Point> *>(dest))[elementIndex] = newValue.As<Point>();
            break;
        case Variant::Type::Rect:
            (*reinterpret_cast<Array<Rect> *>(dest))[elementIndex] = newValue.As<Rect>();
            break;
        case Variant::Type::Guid:
            (*reinterpret_cast<Array<Guid> *>(dest))[elementIndex] = newValue.As<Guid>();
            break;
        case Variant::Type::Str:
            (*reinterpret_cast<Array<Str> *>(dest))[elementIndex] = newValue.As<Str>();
            break;
        case Variant::Type::MinMaxCurve:
            (*reinterpret_cast<Array<MinMaxCurve> *>(dest))[elementIndex] = newValue.As<MinMaxCurve>();
            break;
        default:
            assert(0);
            break;
        }
    }

    EmitSignal(&Serializable::SIG_PropertyChanged, propertyInfo.name.c_str(), elementIndex);
    return true;
}

int Serializable::GetPropertyArrayCount(const char *name) const {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::GetPropertyArrayCount: invalid property name '%s'\n", name);
        return 0;
    }

    return GetPropertyArrayCount(propertyInfo);
}

int Serializable::GetPropertyArrayCount(int index) const {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::GetPropertyArrayCount: invalid property index %i\n", index);
        return 0;
    }

    return GetPropertyArrayCount(propertyInfo);
}

int Serializable::GetPropertyArrayCount(const PropertyInfo &propertyInfo) const {
    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::Array)) {
        BE_WARNLOG("Serializable::GetPropertyArrayCount: property '%s' is not array\n", propertyInfo.name.c_str());
        return 0;
    }

    if (propertyInfo.accessor) {
        return propertyInfo.accessor->GetCount(this);
    }

    const void *src = reinterpret_cast<const byte *>(this) + propertyInfo.offset;
    return reinterpret_cast<const Array<int> *>(src)->Count();
}

void Serializable::SetPropertyArrayCount(const char *name, int count) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG("Serializable::SetPropertyArrayCount: invalid property name '%s'\n", name);
        return;
    }

    SetPropertyArrayCount(propertyInfo, count);
}

void Serializable::SetPropertyArrayCount(int index, int count) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(index, propertyInfo)) {
        BE_WARNLOG("Serializable::SetPropertyArrayCount: invalid property index %i'\n", index);
        return;
    }

    SetPropertyArrayCount(propertyInfo, count);
}

void Serializable::SetPropertyArrayCount(const PropertyInfo &propertyInfo, int count) {
    if (!(propertyInfo.GetFlags() & PropertyInfo::Flag::Array)) {
        BE_WARNLOG("Serializable::SetPropertyArrayCount: property '%s' is not array\n", propertyInfo.name.c_str());
        return;
    }

    if (propertyInfo.GetFlags() & PropertyInfo::Flag::ReadOnly) {
        BE_WARNLOG("Serializable::SetPropertyArrayCount: property '%s' is readonly\n", propertyInfo.name.c_str());
        return;
    }

    int oldCount = 0;

    if (propertyInfo.accessor) {
        oldCount = propertyInfo.accessor->GetCount(this);
        if (oldCount == count) {
            return;
        }

        propertyInfo.accessor->SetCount(this, count);
    } else {
        void *src = reinterpret_cast<byte *>(this) + propertyInfo.offset;

        switch (propertyInfo.GetType()) {
        case Variant::Type::Int:
            oldCount = reinterpret_cast<Array<int> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<int> *>(src)->SetCount(count);
            break;
        case Variant::Type::Int64:
            oldCount = reinterpret_cast<Array<int64_t> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<int64_t> *>(src)->SetCount(count);
            break;
        case Variant::Type::Bool:
            oldCount = reinterpret_cast<Array<bool> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<bool> *>(src)->SetCount(count);
            break;
        case Variant::Type::Float:
            oldCount = reinterpret_cast<Array<float> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<float> *>(src)->SetCount(count);
            break;
        case Variant::Type::Double:
            oldCount = reinterpret_cast<Array<double> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<double> *>(src)->SetCount(count);
            break;
        case Variant::Type::Vec2:
            oldCount = reinterpret_cast<Array<Vec2> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec2> *>(src)->SetCount(count);
            break;
        case Variant::Type::Vec3:
            oldCount = reinterpret_cast<Array<Vec3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec3> *>(src)->SetCount(count);
            break;
        case Variant::Type::Vec4:
            oldCount = reinterpret_cast<Array<Vec4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec4> *>(src)->SetCount(count);
            break;
        case Variant::Type::Color3:
            oldCount = reinterpret_cast<Array<Color3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Color3> *>(src)->SetCount(count);
            break;
        case Variant::Type::Color4:
            oldCount = reinterpret_cast<Array<Color4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Color4> *>(src)->SetCount(count);
            break;
        case Variant::Type::Angles:
            oldCount = reinterpret_cast<Array<Angles> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Angles> *>(src)->SetCount(count);
            break;
        case Variant::Type::Quat:
            oldCount = reinterpret_cast<Array<Quat> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Quat> *>(src)->SetCount(count);
            break;
        case Variant::Type::Mat2:
            oldCount = reinterpret_cast<Array<Mat2> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat2> *>(src)->SetCount(count);
            break;
        case Variant::Type::Mat3:
            oldCount = reinterpret_cast<Array<Mat3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat3> *>(src)->SetCount(count);
            break;
        case Variant::Type::Mat3x4:
            oldCount = reinterpret_cast<Array<Mat3x4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat3x4> *>(src)->SetCount(count);
            break;
        case Variant::Type::Mat4:
            oldCount = reinterpret_cast<Array<Mat4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat4> *>(src)->SetCount(count);
            break;
        case Variant::Type::Point:
            oldCount = reinterpret_cast<Array<Point> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Point> *>(src)->SetCount(count);
            break;
        case Variant::Type::Rect:
            oldCount = reinterpret_cast<Array<Rect> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Rect> *>(src)->SetCount(count);
            break;
        case Variant::Type::Guid:
            oldCount = reinterpret_cast<Array<Guid> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Guid> *>(src)->SetCount(count);
            break;
        case Variant::Type::Str:
            oldCount = reinterpret_cast<Array<Str> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Str> *>(src)->SetCount(count);
            break;
        case Variant::Type::MinMaxCurve:
            oldCount = reinterpret_cast<Array<MinMaxCurve> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<MinMaxCurve> *>(src)->SetCount(count);
            break;
        default:
            assert(0);
            break;
        }

        BlockSignals(true);

        if (count > oldCount) {
            for (int elementIndex = oldCount; elementIndex < count; elementIndex++) {
                SetArrayProperty(propertyInfo, elementIndex, propertyInfo.defaultValue, true);
            }
        }

        BlockSignals(false);
    }

    if (count != oldCount) {
        EmitSignal(&SIG_PropertyArrayCountChanged, propertyInfo.name.c_str());
    }
}

BE_NAMESPACE_END
