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
const SignalDef Serializable::SIG_PropertyUpdated("Serializable::PropertyUpdated");

bool Serializable::GetPropertyInfo(int index, PropertyInfo &propertyInfo) const {
    Array<PropertyInfo> propertyInfos;
    GetPropertyInfoList(propertyInfos);

    if (index < 0 || index > propertyInfos.Count() - 1) {
        return false;
    }

    propertyInfo = propertyInfos[index];
    return true;
}

bool Serializable::GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const {
    Array<PropertyInfo> propertyInfos;
    GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        if (!Str::Cmp(propertyInfos[i].GetName(), name)) {
            propertyInfo = propertyInfos[i];
            return true;
        }
    }

    return false;
}

void Serializable::Serialize(Json::Value &out) const {
    Array<PropertyInfo> propertyInfos;

    GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propertyInfo = propertyInfos[i];

        if (propertyInfo.GetFlags() & PropertyInfo::SkipSerialization) {
            continue;
        }

        const Str name = propertyInfo.GetName();

        if (propertyInfo.GetFlags() & PropertyInfo::IsArray) {
            out[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < GetPropertyArrayCount(name); elementIndex++) {
                Variant var = GetProperty(name, elementIndex);

                out[name][elementIndex] = var.ToJsonValue();
            }
        } else {
            Variant var = GetProperty(name);

            out[name] = var.ToJsonValue();
        }
    }
}

void Serializable::Deserialize(const Json::Value &node) {
    Array<PropertyInfo> propertyInfos;

    GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propertyInfo = propertyInfos[i];

        if (propertyInfo.GetFlags() & PropertyInfo::ReadOnly) {
            continue;
        }

        const Str name = propertyInfo.GetName();
        const Variant::Type type = propertyInfo.GetType();
        const Variant defaultValue = propertyInfo.GetDefaultValue();

        if (propertyInfo.GetFlags() & PropertyInfo::IsArray) {
            const Json::Value subNode = node.get(name.c_str(), Json::Value());

            SetPropertyArrayCount(name, subNode.size());

            for (int elementIndex = 0; elementIndex < subNode.size(); elementIndex++) {
                switch (type) {
                case Variant::IntType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<int>());
                    SetProperty(name, elementIndex, value.asInt());
                    break;
                }
                case Variant::Int64Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<int64_t>());
                    SetProperty(name, elementIndex, value.asInt64());
                    break;
                }
                case Variant::BoolType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<bool>());
                    SetProperty(name, elementIndex, value.asBool());
                    break; 
                }
                case Variant::FloatType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<float>());
                    SetProperty(name, elementIndex, value.asFloat());
                    break; 
                }
                case Variant::Vec2Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec2>().ToString());
                    const char *s = value.asCString();
                    Vec2 v;
                    sscanf(s, "%f %f", &v.x, &v.y);
                    SetProperty(name, elementIndex, v);
                    break; 
                }
                case Variant::Vec3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec3>().ToString());
                    const char *s = value.asCString();
                    Vec3 v;
                    sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                    SetProperty(name, elementIndex, v);
                    break; 
                }
                case Variant::Vec4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Vec4>().ToString());
                    const char *s = value.asCString();
                    Vec4 v;
                    sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                    SetProperty(name, elementIndex, v);
                    break; 
                }
                case Variant::Color3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Color3>().ToString());
                    const char *s = value.asCString();
                    Color3 v;
                    sscanf(s, "%f %f %f", &v.r, &v.g, &v.b);
                    SetProperty(name, elementIndex, v);
                    break; 
                }
                case Variant::Color4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Color4>().ToString());
                    const char *s = value.asCString();
                    Color4 v;
                    sscanf(s, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
                    SetProperty(name, elementIndex, v);
                    break; 
                }
                case Variant::Mat2Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat2>().ToString());
                    const char *s = value.asCString();
                    Mat2 m;
                    sscanf(s, "%f %f %f %f", &m[0].x, &m[0].y, &m[1].x, &m[1].y);
                    SetProperty(name, elementIndex, m);
                    break;
                }
                case Variant::Mat3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat3>().ToString());
                    const char *s = value.asCString();
                    Mat3 m;
                    sscanf(s, "%f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);
                    SetProperty(name, elementIndex, m);
                    break;
                }
                case Variant::Mat3x4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat3x4>().ToString());
                    const char *s = value.asCString();
                    Mat3x4 m;
                    sscanf(s, "%f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w);
                    SetProperty(name, elementIndex, m);
                    break;
                }
                case Variant::Mat4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Mat4>().ToString());
                    const char *s = value.asCString();
                    Mat4 m;
                    sscanf(s, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w, &m[3].x, &m[3].y, &m[3].z, &m[3].w);
                    SetProperty(name, elementIndex, m);
                    break;
                }
                case Variant::AnglesType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Angles>().ToString());
                    const char *s = value.asCString();
                    Angles a;
                    sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                    SetProperty(name, elementIndex, a);
                    break; 
                }
                case Variant::QuatType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Quat>().ToString());
                    const char *s = value.asCString();
                    Quat q;
                    sscanf(s, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w);
                    SetProperty(name, elementIndex, q);
                    break;
                }
                case Variant::PointType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Point>().ToString());
                    const char *s = value.asCString();
                    Point p;
                    sscanf(s, "%i %i", &p.x, &p.y);
                    SetProperty(name, elementIndex, p);
                    break;
                }
                case Variant::RectType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Rect>().ToString());
                    const char *s = value.asCString();
                    Rect r;
                    sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                    SetProperty(name, elementIndex, r);
                    break;
                }
                case Variant::GuidType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Guid>().ToString());
                    Guid guid = Guid::FromString(value.asCString());
                    SetProperty(name, elementIndex, guid);
                    break;
                }
                case Variant::StrType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue.As<Str>().c_str());
                    SetProperty(name, elementIndex, Str(value.asCString()));
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
            case Variant::IntType:
                SetProperty(name, value.asInt());
                break;
            case Variant::Int64Type:
                SetProperty(name, value.asInt64());
                break;
            case Variant::BoolType:
                SetProperty(name, value.asBool());
                break;
            case Variant::FloatType:
                SetProperty(name, value.asFloat());
                break;
            case Variant::Vec2Type: {
                const char *s = value.asCString();
                Vec2 v;
                sscanf(s, "%f %f", &v.x, &v.y);
                SetProperty(name, v);
                break;
            }
            case Variant::Vec3Type: {
                const char *s = value.asCString();
                Vec3 v;
                sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                SetProperty(name, v);
                break;
            }
            case Variant::Vec4Type: {
                const char *s = value.asCString();
                Vec4 v;
                sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                SetProperty(name, v);
                break;
            }
            case Variant::Color3Type: {
                const char *s = value.asCString();
                Color3 v;
                sscanf(s, "%f %f %f", &v.r, &v.g, &v.b);
                SetProperty(name, v);
                break;
            }
            case Variant::Color4Type: {
                const char *s = value.asCString();
                Color4 v;
                sscanf(s, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
                SetProperty(name, v);
                break;
            }
            case Variant::Mat2Type: {
                const char *s = value.asCString();
                Mat2 m;
                sscanf(s, "%f %f %f %f", &m[0].x, &m[0].y, &m[1].x, &m[1].y);
                SetProperty(name, m);
                break;
            }
            case Variant::Mat3Type: {
                const char *s = value.asCString();
                Mat3 m;
                sscanf(s, "%f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);
                SetProperty(name, m);
                break;
            }
            case Variant::Mat3x4Type: {
                const char *s = value.asCString();
                Mat3x4 m;
                sscanf(s, "%f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w);
                SetProperty(name, m);
                break;
            }
            case Variant::Mat4Type: {
                const char *s = value.asCString();
                Mat4 m;
                sscanf(s, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[0].w, &m[1].x, &m[1].y, &m[1].z, &m[1].w, &m[2].x, &m[2].y, &m[2].z, &m[2].w, &m[3].x, &m[3].y, &m[3].z, &m[3].w);
                SetProperty(name, m);
                break;
            }
            case Variant::AnglesType: {
                const char *s = value.asCString();
                Angles a;
                sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                SetProperty(name, a);
                break;
            }
            case Variant::QuatType: {
                const char *s = value.asCString();
                Quat q;
                sscanf(s, "%f %f %f %f", &q.x, &q.y, &q.z, &q.w);
                SetProperty(name, q);
                break;
            }
            case Variant::PointType: {
                const char *s = value.asCString();
                Point p;
                sscanf(s, "%i %i", &p.x, &p.y);
                SetProperty(name, p);
                break;
            }
            case Variant::RectType: {
                const char *s = value.asCString();
                Rect r;
                sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                SetProperty(name, r);
                break;
            }
            case Variant::GuidType: {
                Guid guid = Guid::FromString(value.asCString());
                SetProperty(name, guid);
                break;
            }
            case Variant::StrType:
                SetProperty(name, Str(value.asCString()));
                break;
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
        BE_WARNLOG(L"Serializable::GetPropertyDefault: invalid property name '%hs'\n", name);
        return out;
    }

    out = propertyInfo.GetDefaultValue();
    return out;
}

Variant Serializable::GetProperty(const char *name) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG(L"Serializable::GetProperty: invalid property name '%hs'\n", name);
        return out;
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Get(this, out);
        return out;
    }

    const void *src = reinterpret_cast<const byte *>(this) + propertyInfo.offset;

    switch (propertyInfo.GetType()) {
    case Variant::IntType:
        out = *(reinterpret_cast<const int *>(src));
        break;
    case Variant::Int64Type:
        out = *(reinterpret_cast<const int64_t *>(src));
        break;
    case Variant::BoolType:
        out = *(reinterpret_cast<const bool *>(src));
        break;
    case Variant::FloatType:
        out = *(reinterpret_cast<const float *>(src));
        break;
    case Variant::Vec2Type:
        out = *(reinterpret_cast<const Vec2 *>(src));
        break;
    case Variant::Vec3Type:
        out = *(reinterpret_cast<const Vec3 *>(src));
        break;
    case Variant::Vec4Type:
        out = *(reinterpret_cast<const Vec4 *>(src));
        break;
    case Variant::Color3Type:
        out = *(reinterpret_cast<const Color3 *>(src));
        break;
    case Variant::Color4Type:
        out = *(reinterpret_cast<const Color4 *>(src));
        break;
    case Variant::AnglesType:
        out = *(reinterpret_cast<const Angles *>(src));
        break;
    case Variant::Mat2Type:
        out = *(reinterpret_cast<const Mat2 *>(src));
        break;
    case Variant::Mat3Type:
        out = *(reinterpret_cast<const Mat3 *>(src));
        break;
    case Variant::Mat3x4Type:
        out = *(reinterpret_cast<const Mat3x4 *>(src));
        break;
    case Variant::Mat4Type:
        out = *(reinterpret_cast<const Mat4 *>(src));
        break;
    case Variant::PointType:
        out = *(reinterpret_cast<const Point *>(src));
        break;
    case Variant::RectType:
        out = *(reinterpret_cast<const Rect *>(src));
        break;
    case Variant::GuidType:
        out = *(reinterpret_cast<const Guid *>(src));
        break;
    case Variant::StrType:
        out = *(reinterpret_cast<const Str *>(src));
        break;
    case Variant::MinMaxCurveType:
        out = *(reinterpret_cast<const MinMaxCurve *>(src));
        break;
    }

    return out;
}

Variant Serializable::GetProperty(const char *name, int index) const {
    PropertyInfo propertyInfo;
    Variant out;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG(L"Serializable::GetProperty: invalid property name '%hs'\n", name);
        return out;
    }

    if (!(propertyInfo.flags & PropertyInfo::IsArray)) {
        BE_WARNLOG(L"Serializable::GetProperty: property '%hs' is not array\n", name);
        return out;
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Get(this, index, out);
        return out;
    }

    const void *src = reinterpret_cast<const byte *>(this) + propertyInfo.offset;

    switch (propertyInfo.GetType()) {
    case Variant::IntType:
        out = (*reinterpret_cast<const Array<int> *>(src))[index];
        break;
    case Variant::Int64Type:
        out = (*reinterpret_cast<const Array<int64_t> *>(src))[index];
        break;
    case Variant::BoolType:
        out = (*reinterpret_cast<const Array<bool> *>(src))[index];
        break;
    case Variant::FloatType:
        out = (*reinterpret_cast<const Array<float> *>(src))[index];
        break;
    case Variant::Vec2Type:
        out = (*reinterpret_cast<const Array<Vec2> *>(src))[index];
        break;
    case Variant::Vec3Type:
        out = (*reinterpret_cast<const Array<Vec3> *>(src))[index];
        break;
    case Variant::Vec4Type:
        out = (*reinterpret_cast<const Array<Vec4> *>(src))[index];
        break;
    case Variant::Color3Type:
        out = (*reinterpret_cast<const Array<Color3> *>(src))[index];
        break;
    case Variant::Color4Type:
        out = (*reinterpret_cast<const Array<Color4> *>(src))[index];
        break;
    case Variant::AnglesType:
        out = (*reinterpret_cast<const Array<Angles> *>(src))[index];
        break;
    case Variant::Mat2Type:
        out = (*reinterpret_cast<const Array<Mat2> *>(src))[index];
        break;
    case Variant::Mat3Type:
        out = (*reinterpret_cast<const Array<Mat3> *>(src))[index];
        break;
    case Variant::Mat3x4Type:
        out = (*reinterpret_cast<const Array<Mat3x4> *>(src))[index];
        break;
    case Variant::Mat4Type:
        out = (*reinterpret_cast<const Array<Mat4> *>(src))[index];
        break;
    case Variant::PointType:
        out = (*reinterpret_cast<const Array<Point> *>(src))[index];
        break;
    case Variant::RectType:
        out = (*reinterpret_cast<const Array<Rect> *>(src))[index];
        break;
    case Variant::GuidType:
        out = (*reinterpret_cast<const Array<Guid> *>(src))[index];
        break;
    case Variant::StrType:
        out = (*reinterpret_cast<const Array<Str> *>(src))[index];
        break;
    case Variant::MinMaxCurveType:
        out = (*reinterpret_cast<const Array<MinMaxCurve> *>(src))[index];
        break;
    }

    return out;
}

bool Serializable::SetPropertyWithoutSignal(const char *name, const Variant &var) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG(L"Serializable::SetPropertyWithoutSignal: invalid property name '%hs'\n", name);
        return false;
    }

    // You can force to write a value even though a property has read only flag.
    if (propertyInfo.GetFlags() & PropertyInfo::ReadOnly) {
        BE_WARNLOG(L"Serializable::SetPropertyWithoutSignal: property '%hs' is readonly\n", name);
        return false;
    }

    Variant newVar;
    float minValue;
    float maxValue;

    if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
        minValue = propertyInfo.GetMinValue();
        maxValue = propertyInfo.GetMaxValue();
    }

    switch (propertyInfo.GetType()) {
    case Variant::IntType: {
        int i = var.As<int>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newVar = i;
        break; 
    }
    case Variant::Int64Type: {
        int64_t i = var.As<int64_t>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(i, (int64_t)minValue, (int64_t)maxValue);
        }
        newVar = i;
        break;
    }
    case Variant::BoolType:
        newVar = var.As<bool>();
        break;
    case Variant::FloatType: {
        float f = var.As<float>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(f, minValue, maxValue);
        }
        newVar = f;
        break;
    }
    case Variant::Vec2Type: {
        Vec2 vec2 = var.As<Vec2>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newVar = vec2;
        break; 
    }
    case Variant::Vec3Type: {
        Vec3 vec3 = var.As<Vec3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newVar = vec3;
        break; 
    }
    case Variant::Vec4Type: {
        Vec4 vec4 = var.As<Vec4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newVar = vec4;
        break; 
    }
    case Variant::Color3Type: {
        Color3 color3 = var.As<Color3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color3.r, minValue, maxValue);
            Clamp(color3.g, minValue, maxValue);
            Clamp(color3.b, minValue, maxValue);
        }
        newVar = color3;
        break; 
    }
    case Variant::Color4Type: {
        Color4 color4 = var.As<Color4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color4.r, minValue, maxValue);
            Clamp(color4.g, minValue, maxValue);
            Clamp(color4.b, minValue, maxValue);
            Clamp(color4.a, minValue, maxValue);
        }
        newVar = color4;
        break; 
    }
    case Variant::Mat2Type:
        newVar = var.As<Mat2>();
        break;
    case Variant::Mat3Type:
        newVar = var.As<Mat3>();
        break;
    case Variant::Mat3x4Type:
        newVar = var.As<Mat3x4>();
        break;
    case Variant::Mat4Type:
        newVar = var.As<Mat4>();
        break;
    case Variant::AnglesType: {
        Angles angles = var.As<Angles>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newVar = angles;
        break;
    }
    case Variant::PointType: {
        Point pt = var.As<Point>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newVar = pt;
        break;
    }
    case Variant::RectType: {
        Rect rect = var.As<Rect>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newVar = rect;
        break;
    }
    case Variant::GuidType:
        newVar = var.As<Guid>();
        break;
    case Variant::StrType:
        newVar = var.As<Str>();
        break;
    case Variant::MinMaxCurveType:
        newVar = var.As<MinMaxCurve>();
        break;
    default:
        assert(0);
        break;
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Set(this, newVar);
    } else {
        void *dest = reinterpret_cast<byte *>(this) + propertyInfo.offset;

        switch (propertyInfo.GetType()) {
        case Variant::IntType:
            *(reinterpret_cast<int *>(dest)) = newVar.As<int>();
            break;
        case Variant::Int64Type:
            *(reinterpret_cast<int64_t *>(dest)) = newVar.As<int64_t>();
            break;
        case Variant::BoolType: 
            *(reinterpret_cast<bool *>(dest)) = newVar.As<bool>();
            break;
        case Variant::FloatType:
            *(reinterpret_cast<float *>(dest)) = newVar.As<float>();
            break;
        case Variant::Vec2Type:
            *(reinterpret_cast<Vec2 *>(dest)) = newVar.As<Vec2>();
            break;
        case Variant::Vec3Type:
            *(reinterpret_cast<Vec3 *>(dest)) = newVar.As<Vec3>();
            break;
        case Variant::Vec4Type:
            *(reinterpret_cast<Vec4 *>(dest)) = newVar.As<Vec4>();
            break;
        case Variant::Color3Type:
            *(reinterpret_cast<Color3 *>(dest)) = newVar.As<Color3>();
            break;
        case Variant::Color4Type:
            *(reinterpret_cast<Color4 *>(dest)) = newVar.As<Color4>();
            break;
        case Variant::Mat2Type: 
            *(reinterpret_cast<Mat2 *>(dest)) = newVar.As<Mat2>();
            break;
        case Variant::Mat3Type:
            *(reinterpret_cast<Mat3 *>(dest)) = newVar.As<Mat3>();
            break;
        case Variant::Mat3x4Type:
            *(reinterpret_cast<Mat3x4 *>(dest)) = newVar.As<Mat3x4>();
            break;
        case Variant::Mat4Type:
            *(reinterpret_cast<Mat4 *>(dest)) = newVar.As<Mat4>();
            break;
        case Variant::AnglesType:
            *(reinterpret_cast<Angles *>(dest)) = newVar.As<Angles>();
            break;
        case Variant::QuatType: 
            *(reinterpret_cast<Quat *>(dest)) = newVar.As<Quat>();
            break;
        case Variant::PointType:
            *(reinterpret_cast<Point *>(dest)) = newVar.As<Point>();
            break;
        case Variant::RectType:
            *(reinterpret_cast<Rect *>(dest)) = newVar.As<Rect>();
            break;
        case Variant::GuidType:
            *(reinterpret_cast<Guid *>(dest)) = newVar.As<Guid>();
            break;
        case Variant::StrType:
            *(reinterpret_cast<Str *>(dest)) = newVar.As<Str>();
            break;
        case Variant::MinMaxCurveType:
            *(reinterpret_cast<MinMaxCurve *>(dest)) = newVar.As<MinMaxCurve>();
            break;
        }
    }

    return true;
}

bool Serializable::SetProperty(const char *name, const Variant &var) {
    if (SetPropertyWithoutSignal(name, var)) {
        EmitSignal(&Serializable::SIG_PropertyChanged, name, -1);
        return true;
    }
    return false;
}

bool Serializable::SetProperty(const char *name, int index, const Variant &var) {
    if (SetPropertyWithoutSignal(name, index, var)) {
        EmitSignal(&Serializable::SIG_PropertyChanged, name, index);
        return true;
    }

    return false;
}

bool Serializable::SetPropertyWithoutSignal(const char *name, int index, const Variant &var) {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG(L"Serializable::SetPropertyWithoutSignal: invalid property name '%hs'\n", name);
        return false;
    }

    if (!(propertyInfo.flags & PropertyInfo::IsArray)) {
        BE_WARNLOG(L"Serializable::SetPropertyWithoutSignal: property '%hs' is not array\n", name);
        return false;
    }

    if (propertyInfo.GetFlags() & PropertyInfo::ReadOnly) {
        BE_WARNLOG(L"Serializable::SetPropertyWithoutSignal: property '%hs' is readonly\n", name);
        return false;
    }

    Variant newVar;
    float minValue;
    float maxValue;

    if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
        minValue = propertyInfo.GetMinValue();
        maxValue = propertyInfo.GetMaxValue();
    }

    switch (propertyInfo.GetType()) {
    case Variant::IntType: {
        int i = var.As<int>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newVar = i;
        break; 
    }
    case Variant::Int64Type: {
        int64_t i = var.As<int64_t>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(i, (int64_t)minValue, (int64_t)maxValue);
        }
        newVar = i;
        break;
    }
    case Variant::BoolType:
        newVar = var.As<bool>();
        break;
    case Variant::FloatType: {
        float f = var.As<float>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(f, minValue, maxValue);
        }
        newVar = f;
        break;
    }
    case Variant::Vec2Type: {
        Vec2 vec2 = var.As<Vec2>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newVar = vec2;
        break; 
    }
    case Variant::Vec3Type: {
        Vec3 vec3 = var.As<Vec3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newVar = vec3;
        break; 
    }
    case Variant::Vec4Type: {
        Vec4 vec4 = var.As<Vec4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newVar = vec4;
        break; 
    }
    case Variant::Color3Type: {
        Color3 color3 = var.As<Color3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color3.r, minValue, maxValue);
            Clamp(color3.g, minValue, maxValue);
            Clamp(color3.b, minValue, maxValue);
        }
        newVar = color3;
        break; 
    }
    case Variant::Color4Type: {
        Color4 color4 = var.As<Color4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color4.r, minValue, maxValue);
            Clamp(color4.g, minValue, maxValue);
            Clamp(color4.b, minValue, maxValue);
            Clamp(color4.a, minValue, maxValue);
        }
        newVar = color4;
        break; 
    }
    case Variant::Mat2Type:
        newVar = var.As<Mat2>();
        break;
    case Variant::Mat3Type:
        newVar = var.As<Mat3>();
        break;
    case Variant::Mat3x4Type:
        newVar = var.As<Mat3x4>();
        break;
    case Variant::Mat4Type:
        newVar = var.As<Mat4>();
        break;
    case Variant::AnglesType: {
        Angles angles = var.As<Angles>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newVar = angles;
        break;
    }
    case Variant::PointType: {
        Point pt = var.As<Point>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newVar = pt;
        break;
    }
    case Variant::RectType: {
        Rect rect = var.As<Rect>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newVar = rect;
        break;
    }
    case Variant::GuidType:
        newVar = var.As<Guid>();
        break;
    case Variant::StrType:
        newVar = var.As<Str>();
        break;
    case Variant::MinMaxCurveType:
        newVar = var.As<MinMaxCurve>();
        break;
    default:
        assert(0);
        break;
    }

    if (propertyInfo.accessor) {
        propertyInfo.accessor->Set(this, index, newVar);
    } else {
        void *dest = reinterpret_cast<byte *>(this) + propertyInfo.offset;

        switch (propertyInfo.GetType()) {
        case Variant::IntType:
            (*reinterpret_cast<Array<int> *>(dest))[index] = newVar.As<int>();
            break;
        case Variant::Int64Type:
            (*reinterpret_cast<Array<int64_t> *>(dest))[index] = newVar.As<int64_t>();
            break;
        case Variant::BoolType:
            (*reinterpret_cast<Array<bool> *>(dest))[index] = newVar.As<bool>();
            break;
        case Variant::FloatType:
            (*reinterpret_cast<Array<float> *>(dest))[index] = newVar.As<float>();
            break;
        case Variant::Vec2Type:
            (*reinterpret_cast<Array<Vec2> *>(dest))[index] = newVar.As<Vec2>();
            break;
        case Variant::Vec3Type:
            (*reinterpret_cast<Array<Vec3> *>(dest))[index] = newVar.As<Vec3>();
            break;
        case Variant::Vec4Type:
            (*reinterpret_cast<Array<Vec4> *>(dest))[index] = newVar.As<Vec4>();
            break;
        case Variant::Color3Type:
            (*reinterpret_cast<Array<Color3> *>(dest))[index] = newVar.As<Color3>();
            break;
        case Variant::Color4Type:
            (*reinterpret_cast<Array<Color4> *>(dest))[index] = newVar.As<Color4>();
            break;
        case Variant::Mat2Type: 
            (*reinterpret_cast<Array<Mat2> *>(dest))[index] = newVar.As<Mat2>();
            break;
        case Variant::Mat3Type:
            (*reinterpret_cast<Array<Mat3> *>(dest))[index] = newVar.As<Mat3>();
            break;
        case Variant::Mat3x4Type:
            (*reinterpret_cast<Array<Mat3x4> *>(dest))[index] = newVar.As<Mat3x4>();
            break;
        case Variant::Mat4Type:
            (*reinterpret_cast<Array<Mat4> *>(dest))[index] = newVar.As<Mat4>();
            break;
        case Variant::AnglesType:
            (*reinterpret_cast<Array<Angles> *>(dest))[index] = newVar.As<Angles>();
            break;
        case Variant::QuatType: 
            (*reinterpret_cast<Array<Quat> *>(dest))[index] = newVar.As<Quat>();
            break;
        case Variant::PointType:
            (*reinterpret_cast<Array<Point> *>(dest))[index] = newVar.As<Point>();
            break;
        case Variant::RectType:
            (*reinterpret_cast<Array<Rect> *>(dest))[index] = newVar.As<Rect>();
            break;
        case Variant::GuidType:
            (*reinterpret_cast<Array<Guid> *>(dest))[index] = newVar.As<Guid>();
            break;
        case Variant::StrType:
            (*reinterpret_cast<Array<Str> *>(dest))[index] = newVar.As<Str>();
            break;
        case Variant::MinMaxCurveType:
            (*reinterpret_cast<Array<MinMaxCurve> *>(dest))[index] = newVar.As<MinMaxCurve>();
            break;
        }
    }

    return true;
}

int Serializable::GetPropertyArrayCount(const char *name) const {
    PropertyInfo propertyInfo;

    if (!GetPropertyInfo(name, propertyInfo)) {
        BE_WARNLOG(L"Serializable::GetPropertyArrayCount: invalid property name '%hs'\n", name);
        return 0;
    }

    if (!(propertyInfo.flags & PropertyInfo::IsArray)) {
        BE_WARNLOG(L"Serializable::GetPropertyArrayCount: property '%hs' is not array\n", name);
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
        BE_WARNLOG(L"Serializable::SetPropertyArrayCount: invalid property name '%hs'\n", name);
        return;
    }

    if (!(propertyInfo.flags & PropertyInfo::IsArray)) {
        BE_WARNLOG(L"Serializable::SetPropertyArrayCount: property '%hs' is not array\n", name);
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
        case Variant::IntType:
            oldCount = reinterpret_cast<Array<int> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<int> *>(src)->SetCount(count);
            break;
        case Variant::Int64Type:
            oldCount = reinterpret_cast<Array<int64_t> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<int64_t> *>(src)->SetCount(count);
            break;
        case Variant::BoolType:
            oldCount = reinterpret_cast<Array<bool> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<bool> *>(src)->SetCount(count);
            break;
        case Variant::FloatType:
            oldCount = reinterpret_cast<Array<float> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<float> *>(src)->SetCount(count);
            break;
        case Variant::Vec2Type:
            oldCount = reinterpret_cast<Array<Vec2> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec2> *>(src)->SetCount(count);
            break;
        case Variant::Vec3Type:
            oldCount = reinterpret_cast<Array<Vec3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec3> *>(src)->SetCount(count);
            break;
        case Variant::Vec4Type:
            oldCount = reinterpret_cast<Array<Vec4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Vec4> *>(src)->SetCount(count);
            break;
        case Variant::Color3Type:
            oldCount = reinterpret_cast<Array<Color3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Color3> *>(src)->SetCount(count);
            break;
        case Variant::Color4Type:
            oldCount = reinterpret_cast<Array<Color4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Color4> *>(src)->SetCount(count);
            break;
        case Variant::AnglesType:
            oldCount = reinterpret_cast<Array<Angles> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Angles> *>(src)->SetCount(count);
            break;
        case Variant::Mat2Type:
            oldCount = reinterpret_cast<Array<Mat2> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat2> *>(src)->SetCount(count);
            break;
        case Variant::Mat3Type:
            oldCount = reinterpret_cast<Array<Mat3> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat3> *>(src)->SetCount(count);
            break;
        case Variant::Mat3x4Type:
            oldCount = reinterpret_cast<Array<Mat3x4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat3x4> *>(src)->SetCount(count);
            break;
        case Variant::Mat4Type:
            oldCount = reinterpret_cast<Array<Mat4> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Mat4> *>(src)->SetCount(count);
            break;
        case Variant::PointType:
            oldCount = reinterpret_cast<Array<Point> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Point> *>(src)->SetCount(count);
            break;
        case Variant::RectType:
            oldCount = reinterpret_cast<Array<Rect> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Rect> *>(src)->SetCount(count);
            break;
        case Variant::GuidType:
            oldCount = reinterpret_cast<Array<Guid> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Guid> *>(src)->SetCount(count);
            break;
        case Variant::StrType:
            oldCount = reinterpret_cast<Array<Str> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<Str> *>(src)->SetCount(count);
            break;
        case Variant::MinMaxCurveType:
            oldCount = reinterpret_cast<Array<MinMaxCurve> *>(src)->Count();
            if (oldCount == count) {
                return;
            }
            reinterpret_cast<Array<MinMaxCurve> *>(src)->SetCount(count);
            break;
        }

        if (count > oldCount) {
            for (int index = oldCount; index < count; index++) {
                SetPropertyWithoutSignal(name, index, propertyInfo.defaultValue);
            }
        }
    }

    EmitSignal(&SIG_PropertyArrayCountChanged, name);
}

BE_NAMESPACE_END
