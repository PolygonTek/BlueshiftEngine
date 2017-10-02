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

const PropertySpec *Properties::GetSpec(const char *specname) const {
    const PropertySpec *spec = nullptr;
    char basename[2048];

    if (specname && specname[0]) {
        const char *realSpecName = specname;

        int index = Str::FindChar(specname, '[');
        if (index >= 0) {
            Str::Copynz(basename, specname, index + 1);
            realSpecName = basename;
        }

        spec = owner->FindPropertySpec(realSpecName);
    }

    return spec;
}

const PropertySpec *Properties::GetSpec(int index) const {
    const Str &key = propertyHashMap.GetKey(index);
    const char *name = key.c_str();
    return GetSpec(name);
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
    Array<const PropertySpec *> pspecs;
    owner->GetPropertySpecList(pspecs);    

    for (int i = 0; i < pspecs.Count(); i++) {
        const PropertySpec *spec = pspecs[i];
        const char *key = spec->GetName();
        const auto &value = props->Get(key);

        Set(key, value, true);
    }
}

void Properties::Init(const Json::Value &node) {
    Array<const PropertySpec *> pspecs;
    owner->GetPropertySpecList(pspecs);

    for (int i = 0; i < pspecs.Count(); i++) {
        const PropertySpec *spec = pspecs[i];

        // variable name
        const Str name = spec->GetName();
        // variable type
        const PropertySpec::Type type = spec->GetType();
        // defalut value in C string
        const char *defaultValue = spec->GetDefaultValue();

        if (spec->GetFlags() & PropertySpec::IsArray) {
            Json::Value subNode = node.get(name, Json::Value());

            SetNumElements(name, subNode.size());

            for (int elementIndex = 0; elementIndex < subNode.size(); elementIndex++) {
                const Str elementName = name + va("[%d]", elementIndex);
                
                switch (type) {
                case PropertySpec::StringType: {
                    const Json::Value value = subNode.get(elementIndex, PropertySpec::ToVariant(type, defaultValue).As<Str>().c_str());
                    Set(elementName, value.asCString(), true);
                    break; }
                case PropertySpec::FloatType: {
                    const Json::Value value = subNode.get(elementIndex, PropertySpec::ToVariant(type, defaultValue).As<float>());
                    Set(elementName, value.asFloat(), true);
                    break; }
                case PropertySpec::IntType: 
                case PropertySpec::EnumType: {
                    const Json::Value value = subNode.get(elementIndex, PropertySpec::ToVariant(type, defaultValue).As<int>());
                    Set(elementName, value.asInt(), true);
                    break; }
                case PropertySpec::ObjectType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    Guid guid = Guid::ParseString(value.asCString());
                    Set(elementName, guid, true);
                    break; }
                case PropertySpec::BoolType: {
                    const Json::Value value = subNode.get(elementIndex, PropertySpec::ToVariant(type, defaultValue).As<bool>());
                    Set(elementName, value.asBool(), true);
                    break; }
                case PropertySpec::PointType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Point p;
                    sscanf(s, "%i %i", &p.x, &p.y);
                    Set(elementName, p, true);
                    break; }
                case PropertySpec::RectType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Rect r;
                    sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                    Set(elementName, r, true);
                    break; }
                case PropertySpec::Vec2Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec2 v;
                    sscanf(s, "%f %f", &v.x, &v.y);
                    Set(elementName, v, true);
                    break; }
                case PropertySpec::Vec3Type:
                case PropertySpec::Color3Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec3 v;
                    sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                    Set(elementName, v, true);
                    break; }
                case PropertySpec::Vec4Type:
                case PropertySpec::Color4Type: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Vec4 v;
                    sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                    Set(elementName, v, true);
                    break; }
                case PropertySpec::AnglesType: {
                    const Json::Value value = subNode.get(elementIndex, defaultValue);
                    const char *s = value.asCString();
                    Angles a;
                    sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                    Set(elementName, a, true);
                    break; }
                case PropertySpec::Mat3Type: {
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
            case PropertySpec::StringType: {
                const Json::Value value = node.get(name, PropertySpec::ToVariant(type, defaultValue).As<Str>().c_str());
                Set(name, value.asCString(), true);
                break; }
            case PropertySpec::FloatType: {
                const Json::Value value = node.get(name, PropertySpec::ToVariant(type, defaultValue).As<float>());
                Set(name, value.asFloat(), true);
                break; }
            case PropertySpec::IntType:
            case PropertySpec::EnumType: {
                const Json::Value value = node.get(name, PropertySpec::ToVariant(type, defaultValue).As<int>());
                Set(name, value.asInt(), true);
                break; }
            case PropertySpec::ObjectType: {
                const Json::Value value = node.get(name, defaultValue);
                Guid guid = Guid::ParseString(value.asCString());
                Set(name, guid, true);
                break; }
            case PropertySpec::BoolType: {
                const Json::Value value = node.get(name, PropertySpec::ToVariant(type, defaultValue).As<bool>());
                Set(name, value.asBool(), true);
                break; }
            case PropertySpec::PointType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Point p;
                sscanf(s, "%i %i", &p.x, &p.y);
                Set(name, p, true);
                break; }
            case PropertySpec::RectType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Rect r;
                sscanf(s, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
                Set(name, r, true);
                break; }
            case PropertySpec::Vec2Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec2 v;
                sscanf(s, "%f %f", &v.x, &v.y);
                Set(name, v, true);
                break; }
            case PropertySpec::Vec3Type:
            case PropertySpec::Color3Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec3 v;
                sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
                Set(name, v, true);
                break; }
            case PropertySpec::Vec4Type:
            case PropertySpec::Color4Type: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Vec4 v;
                sscanf(s, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
                Set(name, v, true);
                break; }
            case PropertySpec::AnglesType: {
                const Json::Value value = node.get(name, defaultValue);
                const char *s = value.asCString();
                Angles a;
                sscanf(s, "%f %f %f", &a.yaw, &a.pitch, &a.roll);
                Set(name, a, true);
                break; }
            case PropertySpec::Mat3Type: {
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
    const PropertySpec *spec = GetSpec(name);
    if (!spec) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        out.SetEmpty();
        return false;
    }

    out = PropertySpec::ToVariant(spec->GetType(), spec->GetDefaultValue());
    return true;
}

bool Properties::Get(const char *name, Variant &out, bool forceRead) const {
    const PropertySpec *spec = GetSpec(name);
    if (!spec) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        out.SetEmpty();
        return false;
    }

    if (!forceRead && !(spec->GetFlags() & PropertySpec::Readable)) {
        return false;
    }
    
#ifdef NEW_PROPERTY_SYSTEM
    if (spec->accessor) {
        spec->accessor->Get(owner, out);
        return true;
    }

    const void *src = reinterpret_cast<const byte *>(this) + spec->offset;

    switch (spec->GetType()) {
    case PropertySpec::IntType:
    case PropertySpec::EnumType:    out = *(reinterpret_cast<const int *>(src)); break;
    case PropertySpec::BoolType:    out = *(reinterpret_cast<const bool *>(src)); break;
    case PropertySpec::FloatType:   out = *(reinterpret_cast<const float *>(src)); break;
    case PropertySpec::StringType:  out = *(reinterpret_cast<const Str *>(src)); break;
    case PropertySpec::PointType:   out = *(reinterpret_cast<const Point *>(src)); break;
    case PropertySpec::RectType:    out = *(reinterpret_cast<const Rect *>(src)); break;
    case PropertySpec::Vec2Type:    out = *(reinterpret_cast<const Vec2 *>(src)); break;
    case PropertySpec::Vec3Type:    out = *(reinterpret_cast<const Vec3 *>(src)); break;
    case PropertySpec::Vec4Type:    out = *(reinterpret_cast<const Vec4 *>(src)); break;
    case PropertySpec::Color3Type:  out = *(reinterpret_cast<const Color3 *>(src)); break; 
    case PropertySpec::Color4Type:  out = *(reinterpret_cast<const Color4 *>(src)); break;
    case PropertySpec::AnglesType:  out = *(reinterpret_cast<const Angles *>(src)); break;
    case PropertySpec::Mat3Type:    out = *(reinterpret_cast<const Mat3 *>(src)); break;
    case PropertySpec::ObjectType:  out = *(reinterpret_cast<const Guid *>(src)); break;
    }

    return true;
#else
    const auto *entry = propertyHashMap.Get(name);
    if (!entry) {
        out = PropertySpec::ToVariant(spec->GetType(), spec->GetDefaultValue());
        return true;
    }

    out = entry->second.Value();
    return true;
#endif
}

bool Properties::GetVa(const char *name, ...) const {
    va_list	args;
    
    va_start(args, name);
    while (1) {
        const PropertySpec *spec = GetSpec(name);
        if (!spec) {
            BE_WARNLOG(L"invalid property name '%hs'\n", name);
            va_end(args);
            return false;
        }

        Get(name, *(Variant *)va_arg(args, Variant *));

        name = va_arg(args, const char *);
        if (!name) {
            break;
        }
    }

    va_end(args);
    return true;
}

bool Properties::Set(const char *name, const Variant &var, bool forceWrite) {
    const PropertySpec *spec = GetSpec(name);
    if (!spec) {
        BE_WARNLOG(L"invalid property name '%hs'\n", name);
        return false;
    }

    // You can force to write value even though property has read only flag.
    if (!forceWrite && !(spec->GetFlags() & PropertySpec::Writable)) {
        return false;
    }

    Variant newVar;
    float minValue;
    float maxValue;

    if (spec->GetFlags() & PropertySpec::Ranged) {
        minValue = spec->GetMinValue();
        maxValue = spec->GetMaxValue();
    }

    switch (spec->GetType()) {
    case PropertySpec::StringType: 
        newVar = var.As<Str>();
        break;
    case PropertySpec::FloatType: {
        float f = var.As<float>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(f, minValue, maxValue);
        }
        newVar = f;
        break; }
    case PropertySpec::IntType: {
        int i = var.As<int>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(i, (int)minValue, (int)maxValue);
        }
        newVar = i;
        break; }
    case PropertySpec::EnumType: 
        newVar = var.As<int>();
        break;
    case PropertySpec::ObjectType:
        newVar = var.As<Guid>();
        break;
    case PropertySpec::BoolType:
        newVar = var.As<bool>();
        break;
    case PropertySpec::PointType: {
        Point pt = var.As<Point>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(pt.x, (int)minValue, (int)maxValue);
            Clamp(pt.y, (int)minValue, (int)maxValue);
        }
        newVar = pt;
        break; }
    case PropertySpec::RectType: {
        Rect rect = var.As<Rect>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(rect.x, (int)minValue, (int)maxValue);
            Clamp(rect.y, (int)minValue, (int)maxValue);
            Clamp(rect.w, (int)minValue, (int)maxValue);
            Clamp(rect.h, (int)minValue, (int)maxValue);
        }
        newVar = rect;
        break; }
    case PropertySpec::Vec2Type: {
        Vec2 vec2 = var.As<Vec2>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(vec2.x, minValue, maxValue);
            Clamp(vec2.y, minValue, maxValue);
        }
        newVar = vec2;
        break; }
    case PropertySpec::Vec3Type:
    case PropertySpec::Color3Type: {
        Vec3 vec3 = var.As<Vec3>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(vec3.x, minValue, maxValue);
            Clamp(vec3.y, minValue, maxValue);
            Clamp(vec3.z, minValue, maxValue);
        }
        newVar = vec3;
        break; }
    case PropertySpec::Vec4Type:
    case PropertySpec::Color4Type: {
        Vec4 vec4 = var.As<Vec4>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(vec4.x, minValue, maxValue);
            Clamp(vec4.y, minValue, maxValue);
            Clamp(vec4.z, minValue, maxValue);
            Clamp(vec4.w, minValue, maxValue);
        }
        newVar = vec4;
        break; }
    case PropertySpec::AnglesType: {
        Angles angles = var.As<Angles>();
        if (spec->GetFlags() & PropertySpec::Ranged) {
            Clamp(angles[0], minValue, maxValue);
            Clamp(angles[1], minValue, maxValue);
            Clamp(angles[2], minValue, maxValue);
        }
        newVar = angles;
        break; }
    case PropertySpec::Mat3Type:
        newVar = var.As<Mat3>();
        break;
    default:
        assert(0);
        break;
    }

    Variant oldVar = Get(name);

#ifdef NEW_PROPERTY_SYSTEM
    if (spec->accessor) {
        spec->accessor->Set(owner, newVar);
        return true;
    }

    void *dest = reinterpret_cast<byte *>(this) + spec->offset;

    switch (spec->GetType()) {
    case PropertySpec::IntType:
    case PropertySpec::EnumType:    *(reinterpret_cast<int *>(dest)) = newVar.As<int>(); break;
    case PropertySpec::BoolType:    *(reinterpret_cast<bool *>(dest)) = newVar.As<bool>(); break;
    case PropertySpec::FloatType:   *(reinterpret_cast<float *>(dest)) = newVar.As<float>(); break;
    case PropertySpec::StringType:  *(reinterpret_cast<Str *>(dest)) = newVar.As<Str>(); break;
    case PropertySpec::PointType:   *(reinterpret_cast<Point *>(dest)) = newVar.As<Point>(); break;
    case PropertySpec::RectType:    *(reinterpret_cast<Rect *>(dest)) = newVar.As<Rect>(); break;
    case PropertySpec::Vec2Type:    *(reinterpret_cast<Vec2 *>(dest)) = newVar.As<Vec2>(); break;
    case PropertySpec::Vec3Type:    *(reinterpret_cast<Vec3 *>(dest)) = newVar.As<Vec3>(); break;
    case PropertySpec::Vec4Type:    *(reinterpret_cast<Vec4 *>(dest)) = newVar.As<Vec4>(); break;
    case PropertySpec::Color3Type:  *(reinterpret_cast<Color3 *>(dest)) = newVar.As<Color3>(); break;
    case PropertySpec::Color4Type:  *(reinterpret_cast<Color4 *>(dest)) = newVar.As<Color4>(); break;
    case PropertySpec::AnglesType:  *(reinterpret_cast<Angles *>(dest)) = newVar.As<Angles>(); break;
    case PropertySpec::Mat3Type:    *(reinterpret_cast<Mat3 *>(dest)) = newVar.As<Mat3>(); break;
    case PropertySpec::ObjectType: break;
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

bool Properties::SetVa(const char *name, ...) {
    va_list	args;
    
    va_start(args, name);
    while (1) {
        const PropertySpec *spec = GetSpec(name);
        if (!spec) {
            BE_WARNLOG(L"invalid property name '%hs'\n", name);
            va_end(args);
            return false;
        }

        Set(name, va_arg(args, const Variant *));

        name = va_arg(args, const char *);
        if (!name) {
            break;
        }
    }

    va_end(args);
    return true;
}

const Json::Value Properties::Deserialize() const {
    Array<const PropertySpec *> pspecs;
    Json::Value node;
    
    owner->GetPropertySpecList(pspecs);
 
    for (int i = 0; i < pspecs.Count(); i++) {
        const PropertySpec *spec = pspecs[i];
        const Str name = spec->GetName();

        if (spec->GetFlags() & PropertySpec::IsArray) {
            node[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < NumElements(name); elementIndex++) {
                BE1::Str elementName = name + BE1::va("[%d]", elementIndex);

                Variant var;
                Get(elementName, var, true);

                node[name][elementIndex] = PropertySpec::ToJsonValue(spec->GetType(), var);
            }
        } else {
            Variant var;
            Get(name, var, true);

            node[name] = PropertySpec::ToJsonValue(spec->GetType(), var);
        }
    }

    return node;
}

void Properties::Serialize(Json::Value &out) const {
    Array<const PropertySpec *> pspecs;
    
    owner->GetPropertySpecList(pspecs);

    for (int i = 0; i < pspecs.Count(); i++) {
        const PropertySpec *spec = pspecs[i];
        const Str name = spec->GetName();

        if (spec->GetFlags() & PropertySpec::SkipSerialization) {
            continue;
        }

        if (spec->GetFlags() & PropertySpec::IsArray) {
            out[name] = Json::arrayValue;

            for (int elementIndex = 0; elementIndex < NumElements(name); elementIndex++) {
                BE1::Str elementName = name + BE1::va("[%d]", elementIndex);

                Variant var;
                Get(elementName, var, true);

                out[name][elementIndex] = PropertySpec::ToJsonValue(spec->GetType(), var);
            }
        } else {
            Variant var;
            Get(name, var, true);

            out[name] = PropertySpec::ToJsonValue(spec->GetType(), var);
        }
    }
}

BE_NAMESPACE_END