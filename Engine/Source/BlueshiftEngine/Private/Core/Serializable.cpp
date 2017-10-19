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

const SignalDef Properties::SIG_PropertyChanged("propertyChanged", "ss");
const SignalDef Properties::SIG_PropertyArrayNumChanged("propertyArrayNumChanged", "ss");
const SignalDef Properties::SIG_PropertyFlagsChanged("propertyFlagsChanged", "ss");
const SignalDef Properties::SIG_UpdateUI("updateUI");

Properties::Properties(Object *owner) {
    this->owner = owner;
}

Properties::~Properties() {
    Purge();
}

void Properties::Purge() {
    propertyHashMap.Clear();
}

const char *Properties::GetName(int index) const {
    const Str &key = propertyHashMap.GetKey(index);
    return key.c_str();
}

bool Properties::GetInfo(const char *specname, PropertyInfo &propertyInfo) const {
    char basename[2048];

    if (!specname || !specname[0]) {
        return false;
    }

    const char *realSpecName = specname;

    int index = Str::FindChar(specname, '[');
    if (index >= 0) {
        Str::Copynz(basename, specname, index + 1);
        realSpecName = basename;
    }

    return owner->FindPropertyInfo(realSpecName, propertyInfo);
}

bool Properties::GetInfo(int index, PropertyInfo &propertyInfo) const {
    const Str &key = propertyHashMap.GetKey(index);
    const char *name = key.c_str();
    return GetInfo(name, propertyInfo);
}

int Properties::NumElements(const char *name) const {
    const auto entry = propertyHashMap.Get(name);
    if (!entry) {
        return 0;
    }

    return entry->second.numElements;
}

void Properties::SetNumElements(const char *name, int numElements) {
    auto &prop = propertyHashMap[name];

    if (prop.numElements != numElements) {
        prop.numElements = numElements;

        owner->EmitSignal(&SIG_PropertyArrayNumChanged, owner->ClassName(), name);
    }
}

int Properties::GetFlags(const char *name) const {
    const auto entry = propertyHashMap.Get(name);
    if (!entry) {
        return 0;
    }

    return entry->second.flags;
}

void Properties::SetFlags(const char *name, int flags) {
    const auto entry = propertyHashMap.Get(name);
    if (!entry) {
        return;
    }

    int oldFlags = entry->second.flags;

    entry->second.flags = flags;

    if (oldFlags != flags) {
        owner->EmitSignal(&SIG_PropertyFlagsChanged, owner->ClassName(), name);
    }
}

void Properties::Init(const Properties *props) {
    Array<PropertyInfo> propertyInfos;
    owner->GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propInfo = propertyInfos[i];
        const char *key = propInfo.GetName();
        const auto &value = props->Get(key);

        Set(key, value, true);
    }
}

void Properties::Init(const Json::Value &node) {
    Array<PropertyInfo> propertyInfos;
    owner->GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propInfo = propertyInfos[i];

        // variable name
        const Str name = propInfo.GetName();
        // variable type
        const PropertyInfo::Type type = propInfo.GetType();
        // defalut value in C string
        const char *defaultValue = propInfo.GetDefaultValue();

        if (propInfo.GetFlags() & PropertyInfo::IsArray) {
            Json::Value subNode = node.get(name, Json::Value());

            SetNumElements(name, subNode.size());

            for (int elementIndex = 0; elementIndex < subNode.size(); elementIndex++) {
                const Str elementName = name + va("[%d]", elementIndex);
                
                switch (type) {
                case PropertyInfo::StringType: {
                    Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                    const Json::Value value = subNode.get(elementIndex, variant.As<Str>().c_str());
                    Set(elementName, Str(value.asCString()), true);
                    break; }
                case PropertyInfo::FloatType: {
                    Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                    const Json::Value value = subNode.get(elementIndex, variant.As<float>());
                    Set(elementName, value.asFloat(), true);
                    break; }
                case PropertyInfo::IntType: 
                case PropertyInfo::EnumType: {
                    Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                    const Json::Value value = subNode.get(elementIndex, variant.As<int>());
                    Set(elementName, value.asInt(), true);
                    break; }
                case PropertyInfo::ObjectType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    Guid guid = Guid::FromString(value.asCString());
                    Set(elementName, guid, true);
                    break; }
                case PropertyInfo::BoolType: {
                    Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                    const Json::Value value = subNode.get(elementIndex, variant.As<bool>());
                    Set(elementName, value.asBool(), true);
                    break; }
                case PropertyInfo::PointType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Point p;
                    sscanf(s, "%i %i", &p.x, &p.y);
                    Set(elementName, p, true);
                    break; }
                case PropertyInfo::RectType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Rect r;
                    sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                    Set(elementName, r, true);
                    break; }
                case PropertyInfo::Vec2Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec2 v;
                    sscanf(s, "%f %f", &v.x, &v.y);
                    Set(elementName, v, true);
                    break; }
                case PropertyInfo::Vec3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec3 v;
                    sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                    Set(elementName, v, true);
                    break; }
                case PropertyInfo::Vec4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec4 v;
                    sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                    Set(elementName, v, true);
                    break; }
                case PropertyInfo::Color3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Color3 v;
                    sscanf(s, "%f %f %f", &v.r, &v.g, &v.b);
                    Set(elementName, v, true);
                    break; }
                case PropertyInfo::Color4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Color4 v;
                    sscanf(s, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
                    Set(elementName, v, true);
                    break; }
                case PropertyInfo::AnglesType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Angles a;
                    sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                    Set(elementName, a, true);
                    break; }
                case PropertyInfo::Mat3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Mat3 m;
                    sscanf(s, "%f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);
                    Set(elementName, m, true);
                    break; }
                default:
                    assert(0);
                    break;
                }
            }
        } else {
            switch (type) {
            case PropertyInfo::StringType: {
                Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                const Json::Value value = node.get(name, variant.As<Str>().c_str());
                Set(name, Str(value.asCString()), true);
                break; }
            case PropertyInfo::FloatType: {
                Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                const Json::Value value = node.get(name, variant.As<float>());
                Set(name, value.asFloat(), true);
                break; }
            case PropertyInfo::IntType:
            case PropertyInfo::EnumType: {
                Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                const Json::Value value = node.get(name, variant.As<int>());
                Set(name, value.asInt(), true);
                break; }
            case PropertyInfo::ObjectType: {
                const Json::Value value = node.get(name, defaultValue);
                Guid guid = Guid::FromString(value.asCString());
                Set(name, guid, true);
                break; }
            case PropertyInfo::BoolType: {
                Variant variant = PropertyInfo::ToVariant(type, defaultValue);
                const Json::Value value = node.get(name, variant.As<bool>());
                Set(name, value.asBool(), true);
                break; }
            case PropertyInfo::PointType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Point p;
                sscanf(s, "%i %i", &p.x, &p.y);
                Set(name, p, true);
                break; }
            case PropertyInfo::RectType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Rect r;
                sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                Set(name, r, true);
                break; }
            case PropertyInfo::Vec2Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec2 v;
                sscanf(s, "%f %f", &v.x, &v.y);
                Set(name, v, true);
                break; }
            case PropertyInfo::Vec3Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec3 v;
                sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                Set(name, v, true);
                break; }
            case PropertyInfo::Vec4Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec4 v;
                sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                Set(name, v, true);
                break; }
            case PropertyInfo::Color3Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Color3 v;
                sscanf(s, "%f %f %f", &v.r, &v.g, &v.b);
                Set(name, v, true);
                break; }
            case PropertyInfo::Color4Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Color4 v;
                sscanf(s, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
                Set(name, v, true);
                break; }
            case PropertyInfo::AnglesType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Angles a;
                sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                Set(name, a, true);
                break; }
            case PropertyInfo::Mat3Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Mat3 m;
                sscanf(s, "%f %f %f %f %f %f %f %f %f", &m[0].x, &m[0].y, &m[0].z, &m[1].x, &m[1].y, &m[1].z, &m[2].x, &m[2].y, &m[2].z);
                Set(name, m, true);
                break; }
            default:
                assert(0);
                break;
            }
        }
    }
}

bool Properties::GetDefaultValue(const char *name, Variant &out) const {
    PropertyInfo propInfo;
    if (!GetInfo(name, propInfo)) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        out.Clear();
        return false;
    }

    out = PropertyInfo::ToVariant(propInfo.GetType(), propInfo.GetDefaultValue());
    return true;
}

bool Properties::Get(const char *name, Variant &out, bool forceRead) const {
    PropertyInfo propInfo;
    if (!GetInfo(name, propInfo)) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        out.Clear();
        return false;
    }

    if (!forceRead && !(propInfo.GetFlags() & PropertyInfo::Readable)) {
        return false;
    }
    
#ifdef NEW_PROPERTY_SYSTEM
    if (propInfo->accessor) {
        propInfo->accessor->Get(owner, out);
        return true;
    }

    const void *src = reinterpret_cast<const byte *>(this) + propInfo->offset;

    switch (propInfo->GetType()) {
    case PropertyInfo::IntType:
    case PropertyInfo::EnumType:    out = *(reinterpret_cast<const int *>(src)); break;
    case PropertyInfo::BoolType:    out = *(reinterpret_cast<const bool *>(src)); break;
    case PropertyInfo::FloatType:   out = *(reinterpret_cast<const float *>(src)); break;
    case PropertyInfo::StringType:  out = *(reinterpret_cast<const Str *>(src)); break;
    case PropertyInfo::PointType:   out = *(reinterpret_cast<const Point *>(src)); break;
    case PropertyInfo::RectType:    out = *(reinterpret_cast<const Rect *>(src)); break;
    case PropertyInfo::Vec2Type:    out = *(reinterpret_cast<const Vec2 *>(src)); break;
    case PropertyInfo::Vec3Type:    out = *(reinterpret_cast<const Vec3 *>(src)); break;
    case PropertyInfo::Vec4Type:    out = *(reinterpret_cast<const Vec4 *>(src)); break;
    case PropertyInfo::Color3Type:  out = *(reinterpret_cast<const Color3 *>(src)); break; 
    case PropertyInfo::Color4Type:  out = *(reinterpret_cast<const Color4 *>(src)); break;
    case PropertyInfo::AnglesType:  out = *(reinterpret_cast<const Angles *>(src)); break;
    case PropertyInfo::Mat3Type:    out = *(reinterpret_cast<const Mat3 *>(src)); break;
    case PropertyInfo::ObjectType:  out = *(reinterpret_cast<const Guid *>(src)); break;
    }

    return true;
#else
    const auto *entry = propertyHashMap.Get(name);
    if (!entry) {
        out = PropertyInfo::ToVariant(propInfo.GetType(), propInfo.GetDefaultValue());
        return true;
    }

    out = entry->second.Value();
    return true;
#endif
}

bool Properties::Set(const char *name, const Variant &var, bool forceWrite) {
    PropertyInfo propertyInfo;
    if (!GetInfo(name, propertyInfo)) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        return false;
    }

    // You can force to write value even though property has read only flag.
    if (!forceWrite && !(propertyInfo.GetFlags() & PropertyInfo::Writable)) {
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
    case PropertyInfo::StringType: 
        newVar = var.As<Str>();
        break;
    case PropertyInfo::FloatType: {
        float f = var.As<float>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(f, minValue, maxValue);
        }
        newVar = f;
        break; }
    case PropertyInfo::IntType: {
        int i = var.As<int>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newVar = i;
        break; }
    case PropertyInfo::EnumType:
        newVar = var.As<int>();
        break;
    case PropertyInfo::ObjectType:
        newVar = var.As<Guid>();
        break;
    case PropertyInfo::BoolType:
        newVar = var.As<bool>();
        break;
    case PropertyInfo::PointType: {
        Point pt = var.As<Point>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newVar = pt;
        break; }
    case PropertyInfo::RectType: {
        Rect rect = var.As<Rect>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newVar = rect;
        break; }
    case PropertyInfo::Vec2Type: {
        Vec2 vec2 = var.As<Vec2>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newVar = vec2;
        break; }
    case PropertyInfo::Vec3Type: {
        Vec3 vec3 = var.As<Vec3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newVar = vec3;
        break; }
    case PropertyInfo::Vec4Type: {
        Vec4 vec4 = var.As<Vec4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newVar = vec4;
        break; }
    case PropertyInfo::Color3Type: {
        Color3 color3 = var.As<Color3>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color3.r, minValue, maxValue);
            Clamp(color3.g, minValue, maxValue);
            Clamp(color3.b, minValue, maxValue);
        }
        newVar = color3;
        break; }
    case PropertyInfo::Color4Type: {
        Color4 color4 = var.As<Color4>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(color4.r, minValue, maxValue);
            Clamp(color4.g, minValue, maxValue);
            Clamp(color4.b, minValue, maxValue);
            Clamp(color4.a, minValue, maxValue);
        }
        newVar = color4;
        break; }
    case PropertyInfo::AnglesType: {
        Angles angles = var.As<Angles>();
        if (propertyInfo.GetFlags() & PropertyInfo::Ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newVar = angles;
        break; }
    case PropertyInfo::Mat3Type:
        newVar = var.As<Mat3>();
        break;
    default:
        assert(0);
        break;
    }

    Variant oldVar = Get(name);

#ifdef NEW_PROPERTY_SYSTEM
    if (propInfo->accessor) {
        propInfo->accessor->Set(owner, newVar);
        return true;
    }

    void *dest = reinterpret_cast<byte *>(this) + propInfo->offset;

    switch (propInfo->GetType()) {
    case PropertyInfo::IntType:
    case PropertyInfo::EnumType:    *(reinterpret_cast<int *>(dest)) = newVar.As<int>(); break;
    case PropertyInfo::BoolType:    *(reinterpret_cast<bool *>(dest)) = newVar.As<bool>(); break;
    case PropertyInfo::FloatType:   *(reinterpret_cast<float *>(dest)) = newVar.As<float>(); break;
    case PropertyInfo::StringType:  *(reinterpret_cast<Str *>(dest)) = newVar.As<Str>(); break;
    case PropertyInfo::PointType:   *(reinterpret_cast<Point *>(dest)) = newVar.As<Point>(); break;
    case PropertyInfo::RectType:    *(reinterpret_cast<Rect *>(dest)) = newVar.As<Rect>(); break;
    case PropertyInfo::Vec2Type:    *(reinterpret_cast<Vec2 *>(dest)) = newVar.As<Vec2>(); break;
    case PropertyInfo::Vec3Type:    *(reinterpret_cast<Vec3 *>(dest)) = newVar.As<Vec3>(); break;
    case PropertyInfo::Vec4Type:    *(reinterpret_cast<Vec4 *>(dest)) = newVar.As<Vec4>(); break;
    case PropertyInfo::Color3Type:  *(reinterpret_cast<Color3 *>(dest)) = newVar.As<Color3>(); break;
    case PropertyInfo::Color4Type:  *(reinterpret_cast<Color4 *>(dest)) = newVar.As<Color4>(); break;
    case PropertyInfo::AnglesType:  *(reinterpret_cast<Angles *>(dest)) = newVar.As<Angles>(); break;
    case PropertyInfo::Mat3Type:    *(reinterpret_cast<Mat3 *>(dest)) = newVar.As<Mat3>(); break;
    case PropertyInfo::ObjectType:  break;
    }

    return true;
#else
    propertyHashMap.Set(name, Property(newVar, 0));

    if (oldVar != newVar) {
        owner->EmitSignal(&Properties::SIG_PropertyChanged, owner->ClassName(), name);
    }

    return true;
#endif
}

const Json::Value Properties::Deserialize() const {
    Array<PropertyInfo> propertyInfos;
    Json::Value node;
    
    owner->GetPropertyInfoList(propertyInfos);
 
    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propInfo = propertyInfos[i];
        const Str name = propInfo.GetName();

        if (propInfo.GetFlags() & PropertyInfo::IsArray) {
            node[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < NumElements(name); elementIndex++) {
                BE1::Str elementName = name + BE1::va("[%d]", elementIndex);

                Variant var;
                Get(elementName, var, true);

                node[name][elementIndex] = PropertyInfo::ToJsonValue(propInfo.GetType(), var);
            }
        } else {
            Variant var;
            Get(name, var, true);

            node[name] = PropertyInfo::ToJsonValue(propInfo.GetType(), var);
        }
    }

    return node;
}

void Properties::Serialize(Json::Value &out) const {
    Array<PropertyInfo> propertyInfos;
    
    owner->GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        const PropertyInfo &propInfo = propertyInfos[i];
        const Str name = propInfo.GetName();

        if (propInfo.GetFlags() & PropertyInfo::SkipSerialization) {
            continue;
        }

        if (propInfo.GetFlags() & PropertyInfo::IsArray) {
            out[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < NumElements(name); elementIndex++) {
                BE1::Str elementName = name + BE1::va("[%d]", elementIndex);

                Variant var;
                Get(elementName, var, true);

                out[name][elementIndex] = PropertyInfo::ToJsonValue(propInfo.GetType(), var);
            }
        } else {
            Variant var;
            Get(name, var, true);

            out[name] = PropertyInfo::ToJsonValue(propInfo.GetType(), var);
        }
    }
}

BE_NAMESPACE_END