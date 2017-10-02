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

class BE_API PropertySpec {
    friend class Properties;
    friend class MetaObject;

public:
    /// Property types
    enum Type {
        BadType             = -1,
        IntType,
        BoolType,
        FloatType,
        StringType,
        PointType,
        RectType,
        Vec2Type,
        Vec3Type,
        Vec4Type,
        Color3Type,
        Color4Type,
        AnglesType,
        Mat3Type,
        EnumType,
        ObjectType,
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

    PropertySpec();
    ~PropertySpec();

    PropertySpec(const PropertySpec &pspec);
#ifdef NEW_PROPERTY_SYSTEM
    PropertySpec(const char *name, Type type, int offset, const char *defaultValue, const char *desc, int flags);
    PropertySpec(const char *name, const Enum &e, int offset, const char *defaultValue, const char *desc, int flags);
    PropertySpec(const char *name, const MetaObject &metaObject, int offset, const char *defaultValue, const char *desc, int flags);
    PropertySpec(const char *name, Type type, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags);
    PropertySpec(const char *name, const Enum &e, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags);
    PropertySpec(const char *name, const MetaObject &metaObject, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags);
#else
    PropertySpec(Type type, const char *name, const char *label, const char *desc, const char *defaultValue, int flags);
    PropertySpec(Type type, const char *name, const char *label, const char *desc, const Rangef &r, const char *defaultValue, int flags);
    PropertySpec(Type type, const char *name, const char *label, const char *desc, const Enum &e, const char *defaultValue, int flags);
    PropertySpec(Type type, const char *name, const char *label, const char *desc, const MetaObject &metaObject, const char *defaultValue, int flags);
#endif

    Type                    GetType() const { return type; }
    const char *            GetName() const { return name; }
    const char *            GetDefaultValue() const { return defaultValue; }
    const char *            GetLabel() const { return label; }
    const char *            GetDescription() const { return desc; }
    int                     GetFlags() const { return flags; }
    float                   GetMinValue() const { return range.minValue; }
    float                   GetMaxValue() const { return range.maxValue; }
    float                   GetStep() const { return range.step; }
    const Array<Str> &      GetEnum() const { return enumeration; }
    const MetaObject *      GetMetaObject() const { return metaObject; }

    void                    SetRange(float minValue, float maxValue, float step) { range = Rangef(minValue, maxValue, step); }

    char *                  ToString() const;

    bool                    ParseSpec(Lexer &lexer);

    static const Json::Value ToJsonValue(Type type, const Variant &var);
    static const Variant    ToVariant(Type type, const char *value);

private:
    Type                    type;               ///< Property type
    Str                     name;               ///< Variable name
    Str                     defaultValue;       ///< Default value in Str
    Str                     label;              ///< Label in Editor
    Str                     desc;               ///< Description in Editor
    int                     offset;             ///< Byte offset from start of object
    PropertyAccessor *      accessor;
    int                     flags;
    Rangef                  range;
    Array<Str>              enumeration;        ///< Enumeration string list for enumeration type
    const MetaObject *      metaObject;         ///< MetaObject pointer for object type
};

BE_INLINE PropertySpec::PropertySpec() {
    this->type = BadType;
    this->offset = 0;
    this->accessor = nullptr;
    this->flags = Empty;
    this->range = Rangef(0, 0, 1);
    this->metaObject = nullptr;
}

BE_INLINE PropertySpec::~PropertySpec() {
    if (accessor) {
        delete accessor;
    }
}

BE_INLINE PropertySpec::PropertySpec(const PropertySpec &pspec) {
    this->type = pspec.type;
    this->name = pspec.name;
    this->defaultValue = pspec.defaultValue;
    this->offset = pspec.offset;
    this->accessor = pspec.accessor;
    this->flags = pspec.flags;
    this->label = pspec.label;
    this->desc = pspec.desc;
    this->range = pspec.range;
    this->enumeration = pspec.enumeration;
    this->metaObject = pspec.metaObject;
}

#ifdef NEW_PROPERTY_SYSTEM

BE_INLINE PropertySpec::PropertySpec(const char *name, Type type, int offset, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(const char *name, const Enum &e, int offset, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(const char *name, const MetaObject &metaObject, int offset, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(const char *name, Type type, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(const char *name, const Enum &e, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(const char *name, const MetaObject &metaObject, PropertyAccessor *accesor, const char *defaultValue, const char *desc, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(Type type, const char *name, const char *label, const char *desc, const char *defaultValue, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(Type type, const char *name, const char *label, const char *desc, const Rangef &r, const char *defaultValue, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(Type type, const char *name, const char *label, const char *desc, const Enum &e, const char *defaultValue, int flags) {
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

BE_INLINE PropertySpec::PropertySpec(Type type, const char *name, const char *label, const char *desc, const MetaObject &metaObject, const char *defaultValue, int flags) {
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

template <typename T, bool = std::is_base_of<Object, T>::value>
struct PropertyTypeID { };

template <>
struct PropertyTypeID<int> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::IntType; }
};

template <>
struct PropertyTypeID<unsigned> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::IntType; }
};

template <>
struct PropertyTypeID<bool> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::BoolType; }
};

template <>
struct PropertyTypeID<float> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::FloatType; }
};

template <>
struct PropertyTypeID<Vec2> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Vec2Type; }
};

template <>
struct PropertyTypeID<Vec3> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Vec3Type; }
};

template <>
struct PropertyTypeID<Vec4> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Vec4Type; }
};

template <>
struct PropertyTypeID<Color3> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Color3Type; }
};

template <>
struct PropertyTypeID<Color4> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Color4Type; }
};

template <>
struct PropertyTypeID<Mat3> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::Mat3Type; }
};

template <>
struct PropertyTypeID<Angles> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::AnglesType; }
};

template <>
struct PropertyTypeID<Point> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::PointType; }
};

template <>
struct PropertyTypeID<Rect> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::RectType; }
};

template <>
struct PropertyTypeID<Str> {
    static PropertySpec::Type GetType() { return PropertySpec::Type::StringType; }
};

template <typename T>
struct PropertyTypeID<T, true> {
    static const MetaObject &GetType() { return T::metaObject; }
};

#define REGISTER_PROPERTY(name, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, PropertyTypeID<type>::GetType(), \
        offsetof(Class, var), defaultValue, desc, flags))

#define REGISTER_ENUM_PROPERTY(name, enumSequence, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, BE1::PropertySpec::Enum(enumSequence), \
        offsetof(Class, var), defaultValue, desc, flags))

#define REGISTER_LIST_PROPERTY(name, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, PropertyTypeID<type>::GetType(), \
        offsetof(Class, var), defaultValue, desc, flags | BE1::PropertySpec::IsArray))

#define REGISTER_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_MIXED_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type, BE1::MixedPropertyTrait>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_ENUM_ACCESSOR_PROPERTY(name, enumSequence, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, enumSequence, \
        new BE1::PropertyAccessorImpl<Class, int>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_LIST_ACCESSOR_PROPERTY(name, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertySpec(name, PropertyTypeID<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter), defaultValue, desc, flags | BE1::PropertySpec::IsArray))

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
    BE1::PropertySpec classname::pspecMap[] = {

#define PROPERTY_STRING(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::StringType, name, label, desc, defaultValue, flags)

#define PROPERTY_FLOAT(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::FloatType, name, label, desc, defaultValue, flags)

#define PROPERTY_INT(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::IntType, name, label, desc, defaultValue, flags)

#define PROPERTY_BOOL(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::BoolType, name, label, desc, defaultValue, flags)

#define PROPERTY_POINT(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::PointType, name, label, desc, defaultValue, flags)

#define PROPERTY_RECT(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::RectType, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC2(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec2Type, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC3(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec3Type, name, label, desc, defaultValue, flags)

#define PROPERTY_VEC4(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec4Type, name, label, desc, defaultValue, flags)

#define PROPERTY_ANGLES(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::AnglesType, name, label, desc, defaultValue, flags)

#define PROPERTY_MAT3(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Mat3Type, name, label, desc, defaultValue, flags)

#define PROPERTY_RANGED_INT(name, label, desc, range, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::IntType, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_FLOAT(name, label, desc, range, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::FloatType, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC2(name, label, desc, range, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec2Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC3(name, label, desc, range, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec3Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_RANGED_VEC4(name, label, desc, range, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Vec4Type, name, label, desc, range, defaultValue, flags)

#define PROPERTY_COLOR3(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Color3Type, name, label, desc, BE1::Rangef(0.0f, 1.0f, 0.01f), defaultValue, flags)

#define PROPERTY_COLOR4(name, label, desc, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::Color4Type, name, label, desc, BE1::Rangef(0.0f, 1.0f, 0.01f), defaultValue, flags)

#define PROPERTY_ENUM(name, label, desc, sequence, defaultValue, flags) \
    BE1::PropertySpec(BE1::PropertySpec::EnumType, name, label, desc, BE1::PropertySpec::Enum(sequence), defaultValue, flags)

#define PROPERTY_OBJECT(name, label, desc, defaultValue, metaObject, flags) \
    BE1::PropertySpec(BE1::PropertySpec::ObjectType, name, label, desc, metaObject, defaultValue, flags)

// property definition end
#define END_PROPERTIES BE1::PropertySpec() };

#endif

BE_NAMESPACE_END
