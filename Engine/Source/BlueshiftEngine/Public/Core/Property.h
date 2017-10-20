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

#include "jsoncpp/include/json/json.h"
#include "Core/Guid.h"
#include "Math/Math.h"
#include "Core/Str.h"
#include "Core/Range.h"
#include "Core/Variant.h"
#include "Containers/StrArray.h"

//#define NEW_PROPERTY_SYSTEM

BE_NAMESPACE_BEGIN

class Properties;
class Object;
class MetaObject;
class Lexer;

/// Abstract base class for invoking property getter/setter functions.
class PropertyAccessor {
public:
    /// Get the property.
    virtual void Get(const Object *ptr, Variant &dest) const = 0;
    /// Set the property.
    virtual void Set(Object *ptr, const Variant &src) = 0;
};

/// Property type trait (default use const reference for object type).
template <typename T> 
struct PropertyTrait {
    /// Get function return type.
    using ReturnType = const T &;
    /// Set function parameter type.
    using ParameterType = const T &;
};

/// Int property trait.
template <> 
struct PropertyTrait<int> {
    using ReturnType = int;
    using ParameterType = int;
};

/// unsigned property trait.
template <> 
struct PropertyTrait<unsigned> {
    using ReturnType = unsigned;
    using ParameterType = unsigned;
};

/// Bool property trait.
template <> 
struct PropertyTrait<bool> {
    using ReturnType = bool;
    using ParameterType = bool;
};

/// Float property trait.
template <> 
struct PropertyTrait<float> {
    using ReturnType = float;
    using ParameterType = float;
};

/// Mixed property trait (use const reference for set function only).
template <typename T> 
struct MixedPropertyTrait {
    using ReturnType = T;
    using ParameterType = const T &;
};

/// Actual property type trait
template <typename T, bool = std::is_base_of<Object, T>::value>
struct PropertyTypeTrait {
    using Type = T;
};

/// Actual property type trait (Guid for Object class)
template <typename T>
struct PropertyTypeTrait<T, true> {
    using Type = Guid;
};

/// Template implementation of the property accessor.
template <typename Class, typename Type, template <typename T> class Trait = PropertyTrait> 
class PropertyAccessorImpl : public PropertyAccessor {
public:
    using ActualType = typename PropertyTypeTrait<Type>::Type;
    using GetFunctionPtr = typename Trait<ActualType>::ReturnType (Class::*)() const;
    using SetFunctionPtr = void (Class::*)(typename Trait<ActualType>::ParameterType);

    /// Construct with function pointers.
    PropertyAccessorImpl(GetFunctionPtr getter, SetFunctionPtr setter) :
        getFunction(getter), setFunction(setter) {
        assert(getFunction);
        assert(setFunction);
    }

    /// Invoke getter function.
    virtual void Get(const Object *ptr, Variant &out) const {
        assert(ptr);
        const Class *classPtr = static_cast<const Class *>(ptr);
        out = (classPtr->*getFunction)();
    }

    /// Invoke setter function.
    virtual void Set(Object *ptr, const Variant &in) {
        assert(ptr);
        Class *classPtr = static_cast<Class *>(ptr);
        (classPtr->*setFunction)(in.As<ActualType>());
    }

    /// Class-specific pointer to getter function.
    GetFunctionPtr getFunction;
    /// Class-specific pointer to setter function.
    SetFunctionPtr setFunction;
};

/// Template implementation of the list property accessor.
template <typename Class, typename Type, template <typename T> class Trait = PropertyTrait>
class ListPropertyAccessorImpl : public PropertyAccessor {
public:
    using ActualType = typename PropertyTypeTrait<Type>::Type;
    using GetFunctionPtr = typename Trait<ActualType>::ReturnType(Class::*)(int index) const;
    using SetFunctionPtr = void (Class::*)(int index, typename Trait<ActualType>::ParameterType);

    /// Construct with function pointers.
    ListPropertyAccessorImpl(GetFunctionPtr getter, SetFunctionPtr setter) :
        getFunction(getter), setFunction(setter) {
        assert(getFunction);
        assert(setFunction);
    }

    /// Invoke getter function.
    virtual void Get(const Object *ptr, int index, Variant &out) const {
        assert(ptr);
        const Class *classPtr = static_cast<const Class *>(ptr);
        out = (classPtr->*getFunction)(index);
    }

    /// Invoke setter function.
    virtual void Set(Object *ptr, int index, const Variant &in) {
        assert(ptr);
        Class *classPtr = static_cast<Class *>(ptr);
        (classPtr->*setFunction)(index, in.As<ActualType>());
    }

    /// Class-specific pointer to getter function.
    GetFunctionPtr getFunction;
    /// Class-specific pointer to setter function.
    SetFunctionPtr setFunction;
};

class BE_API PropertyInfo {
    friend class Properties;
    friend class MetaObject;

public:
    /// Property types
    enum Type {
        BadType             = -1,
        BoolType,
        IntType,
        FloatType,
        StringType,
        EnumType,
        PointType,
        RectType,
        Vec2Type,
        Vec3Type,
        Vec4Type,
        Color3Type,
        Color4Type,
        AnglesType,
        Mat3Type,
        GuidType,
        ObjectType,
        ObjectArrayType
    };

    /// Property flags
    enum Flag {
        Empty               = 0,
        Readable            = BIT(0),
        Writable            = BIT(1),
        ReadWrite           = Readable | Writable,
        Hidden              = BIT(2),
        SkipSerialization   = BIT(3),
        Ranged              = BIT(4),
        MultiLines          = BIT(5),
        IsArray             = BIT(6),
        ShaderDefine        = BIT(7),
    };

    struct Enum {
        Enum(const char *sequence) : sequence(sequence) {}
        Str sequence;
    };

    PropertyInfo();
    ~PropertyInfo();

    PropertyInfo(const PropertyInfo &propInfo);
#ifdef NEW_PROPERTY_SYSTEM
    PropertyInfo(const char *name, Type type, int offset, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, const Enum &e, int offset, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, const MetaObject &metaObject, int offset, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, Type type, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, const Enum &e, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, const MetaObject &metaObject, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags);
#else
    PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Variant &defaultValue, int flags);
    PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Rangef &r, const Variant &defaultValue, int flags);
    PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Enum &e, const Variant &defaultValue, int flags);
    PropertyInfo(Type type, const char *name, const char *label, const char *desc, const MetaObject &metaObject, const Variant &defaultValue, int flags);
#endif

    Type                    GetType() const { return type; }
    const char *            GetName() const { return name; }
    const Variant &         GetDefaultValue() const { return defaultValue; }
    const char *            GetLabel() const { return label; }
    const char *            GetDescription() const { return desc; }
    int                     GetFlags() const { return flags; }
    float                   GetMinValue() const { return range.minValue; }
    float                   GetMaxValue() const { return range.maxValue; }
    float                   GetStep() const { return range.step; }
    const Array<Str> &      GetEnum() const { return enumeration; }
    const MetaObject *      GetMetaObject() const { return metaObject; }

    void                    SetRange(float minValue, float maxValue, float step) { range = Rangef(minValue, maxValue, step); }

    //char *                  ToString() const;

    //bool                    Parse(Lexer &lexer);

    friend bool             ParseShaderPropertyInfo(Lexer &lexer, PropertyInfo &propInfo);

    static const Json::Value ToJsonValue(Type type, const Variant &var);
    static Variant          ToVariant(Type type, const char *value);

private:
    Type                    type;               ///< Property type
    Str                     name;               ///< Variable name
    Variant                 defaultValue;
    Str                     label;              ///< Label in Editor
    Str                     desc;               ///< Description in Editor
    int                     offset;             ///< Byte offset from start of object
    PropertyAccessor *      accessor;
    int                     flags;
    Rangef                  range;
    Array<Str>              enumeration;        ///< Enumeration string list for enumeration type
    const MetaObject *      metaObject;         ///< MetaObject pointer for object type
};

BE_INLINE PropertyInfo::PropertyInfo() {
    this->type = BadType;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = Empty;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
}

BE_INLINE PropertyInfo::~PropertyInfo() {
    if (accessor) {
        delete accessor;
    }
}

BE_INLINE PropertyInfo::PropertyInfo(const PropertyInfo &propInfo) {
    this->type = propInfo.type;
    this->name = propInfo.name;
    this->defaultValue = propInfo.defaultValue;
    this->offset = propInfo.offset;
    this->accessor = propInfo.accessor;
    this->flags = propInfo.flags;
    this->label = propInfo.label;
    this->desc = propInfo.desc;
    this->range = propInfo.range;
    this->enumeration = propInfo.enumeration;
    this->metaObject = propInfo.metaObject;
}

#ifdef NEW_PROPERTY_SYSTEM

BE_INLINE PropertyInfo::PropertyInfo(const char *name, Type type, int offset, const Variant &defaultValue, const char *desc, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = offset;
    this->accessor = nullptr;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
}

BE_INLINE PropertyInfo::PropertyInfo(const char *name, const Enum &e, int offset, const Variant &defaultValue, const char *desc, int flags) {
    this->type = Type::EnumType;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = offset;
    this->accessor = nullptr;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
    SplitStringIntoList(this->enumeration, e.sequence, ";");
}

BE_INLINE PropertyInfo::PropertyInfo(const char *name, const MetaObject &metaObject, int offset, const Variant &defaultValue, const char *desc, int flags) {
    this->type = Type::ObjectType;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = offset;
    this->accessor = nullptr;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = &metaObject;
}

BE_INLINE PropertyInfo::PropertyInfo(const char *name, Type type, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = accesor;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
}

BE_INLINE PropertyInfo::PropertyInfo(const char *name, const Enum &e, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = accesor;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
    SplitStringIntoList(this->enumeration, e.sequence, ";");
}

BE_INLINE PropertyInfo::PropertyInfo(const char *name, const MetaObject &metaObject, PropertyAccessor *accesor, const Variant &defaultValue, const char *desc, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = accesor;
    this->flags = flags;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = &metaObject;
}

#else

BE_INLINE PropertyInfo::PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Variant &defaultValue, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = flags;
    this->label = label;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
}

BE_INLINE PropertyInfo::PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Rangef &r, const Variant &defaultValue, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = flags | Ranged;
    this->label = label;
    this->desc = desc;
    this->range = r;
    this->metaObject = nullptr;
}

BE_INLINE PropertyInfo::PropertyInfo(Type type, const char *name, const char *label, const char *desc, const Enum &e, const Variant &defaultValue, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = flags;
    this->label = label;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
    SplitStringIntoList(this->enumeration, e.sequence, ";");
    assert(enumeration.Count() > 0);
}

BE_INLINE PropertyInfo::PropertyInfo(Type type, const char *name, const char *label, const char *desc, const MetaObject &metaObject, const Variant &defaultValue, int flags) {
    this->type = type;
    this->name = name;
    this->defaultValue = defaultValue;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = flags;
    this->label = label;
    this->desc = desc;
    this->range = Rangef(0, 0, 1);
    this->metaObject = &metaObject;
}

#endif

#ifdef NEW_PROPERTY_SYSTEM

template <typename T>
struct PropertyTypeID { };

template <>
struct PropertyTypeID<int> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::IntType; }
};

template <>
struct PropertyTypeID<unsigned> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::IntType; }
};

template <>
struct PropertyTypeID<bool> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::BoolType; }
};

template <>
struct PropertyTypeID<float> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::FloatType; }
};

template <>
struct PropertyTypeID<Vec2> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Vec2Type; }
};

template <>
struct PropertyTypeID<Vec3> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Vec3Type; }
};

template <>
struct PropertyTypeID<Vec4> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Vec4Type; }
};

template <>
struct PropertyTypeID<Color3> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Color3Type; }
};

template <>
struct PropertyTypeID<Color4> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Color4Type; }
};

template <>
struct PropertyTypeID<Mat3> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::Mat3Type; }
};

template <>
struct PropertyTypeID<Angles> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::AnglesType; }
};

template <>
struct PropertyTypeID<Point> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::PointType; }
};

template <>
struct PropertyTypeID<Rect> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::RectType; }
};

template <>
struct PropertyTypeID<Str> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::StringType; }
};

template <>
struct PropertyTypeID<Guid> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::GuidType; }
};

template <>
struct PropertyTypeID<ObjectRef> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::ObjectType; }
};

template <>
struct PropertyTypeID<ObjectRefArray> {
    static PropertyInfo::Type GetType() { return PropertyInfo::Type::ObjectArrayType; }
};

#define REGISTER_PROPERTY(name, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        offsetof(Class, var), defaultValue, desc, flags))

#define REGISTER_ENUM_PROPERTY(name, enumSequence, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, BE1::PropertyInfo::Enum(enumSequence), \
        offsetof(Class, var), defaultValue, desc, flags))

#define REGISTER_LIST_PROPERTY(name, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        offsetof(Class, var), defaultValue, desc, flags | BE1::PropertyInfo::IsArray))

#define REGISTER_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_MIXED_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type, MixedPropertyTrait>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_ENUM_ACCESSOR_PROPERTY(name, enumSequence, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, enumSequence, \
        new BE1::PropertyAccessorImpl<Class, int>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_LIST_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter), defaultValue, desc, flags | BE1::PropertyInfo::IsArray))

#define REGISTER_LIST_MIXED_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type, MixedPropertyTrait>(&Class::getter, &Class::setter), defaultValue, desc, flags | BE1::PropertyInfo::IsArray))

#define BEGIN_PROPERTIES(classname) static int dummy[] = {
#define PROPERTY_STRING(name, label, desc, defaultValue, flags) 0
#define PROPERTY_FLOAT(name, label, desc, defaultValue, flags) 0
#define PROPERTY_INT(name, label, desc, defaultValue, flags) 0
#define PROPERTY_BOOL(name, label, desc, defaultValue, flags) 0
#define PROPERTY_POINT(name, label, desc, defaultValue, flags) 0
#define PROPERTY_RECT(name, label, desc, defaultValue, flags) 0
#define PROPERTY_VEC2(name, label, desc, defaultValue, flags) 0
#define PROPERTY_VEC3(name, label, desc, defaultValue, flags) 0
#define PROPERTY_VEC4(name, label, desc, defaultValue, flags) 0
#define PROPERTY_ANGLES(name, label, desc, defaultValue, flags) 0
#define PROPERTY_MAT3(name, label, desc, defaultValue, flags) 0
#define PROPERTY_RANGED_INT(name, label, desc, range, defaultValue, flags) 0
#define PROPERTY_RANGED_FLOAT(name, label, desc, range, defaultValue, flags) 0
#define PROPERTY_RANGED_VEC2(name, label, desc, range, defaultValue, flags) 0
#define PROPERTY_RANGED_VEC3(name, label, desc, range, defaultValue, flags) 0
#define PROPERTY_RANGED_VEC4(name, label, desc, range, defaultValue, flags) 0
#define PROPERTY_COLOR3(name, label, desc, defaultValue, flags) 0
#define PROPERTY_COLOR4(name, label, desc, defaultValue, flags) 0
#define PROPERTY_ENUM(name, label, desc, sequence, defaultValue, flags) 0
#define PROPERTY_OBJECT(name, label, desc, defaultValue, metaObject, flags) 0
#define END_PROPERTIES 0 };

#else

// property definition begin
#define BEGIN_PROPERTIES(classname) \
    BE1::PropertyInfo classname::propertyInfos[] = {

#define PROPERTY_STRING(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::StringType, name, label, desc, defaultValue, flags)

#define PROPERTY_FLOAT(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::FloatType, name, label, desc, defaultValue, flags)

#define PROPERTY_INT(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::IntType, name, label, desc, defaultValue, flags)

#define PROPERTY_BOOL(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::BoolType, name, label, desc, defaultValue, flags)

#define PROPERTY_POINT(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::PointType, name, label, desc, defaultValue, flags)

#define PROPERTY_RECT(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::RectType, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC2(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec2Type, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC3(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec3Type, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC4(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec4Type, name, label, desc, defaultValue, flags)

#define PROPERTY_ANGLES(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::AnglesType, name, label, desc, defaultValue, flags)

#define PROPERTY_MAT3(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Mat3Type, name, label, desc, defaultValue, flags)

#define PROPERTY_RANGED_INT(name, label, desc, range, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::IntType, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_FLOAT(name, label, desc, range, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::FloatType, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC2(name, label, desc, range, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec2Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC3(name, label, desc, range, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec3Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC4(name, label, desc, range, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Vec4Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_COLOR3(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Color3Type, name, label, desc, BE1::Rangef(0.0f, 1.0f, 0.01f), defaultValue, flags)

#define PROPERTY_COLOR4(name, label, desc, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::Color4Type, name, label, desc, BE1::Rangef(0.0f, 1.0f, 0.01f), defaultValue, flags)

#define PROPERTY_ENUM(name, label, desc, sequence, defaultValue, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::EnumType, name, label, desc, BE1::PropertyInfo::Enum(sequence), defaultValue, flags)

#define PROPERTY_OBJECT(name, label, desc, defaultValue, metaObject, flags) \
    BE1::PropertyInfo(BE1::PropertyInfo::ObjectType, name, label, desc, metaObject, defaultValue, flags)

// property definition end
#define END_PROPERTIES BE1::PropertyInfo() };

#endif

BE_NAMESPACE_END
