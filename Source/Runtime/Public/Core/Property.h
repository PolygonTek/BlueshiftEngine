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

#include "Core/Str.h"
#include "Core/Range.h"
#include "Core/Variant.h"
#include "Containers/StrArray.h"

BE_NAMESPACE_BEGIN

class Variant;
class Serializable;
class MetaObject;
class Lexer;

/// Abstract base class for invoking property getter/setter functions.
class BE_API PropertyAccessor {
public:
    virtual ~PropertyAccessor() = default;

                            /// Get the property.
    virtual void            Get(const Serializable *ptr, Variant &dest) const = 0;

                            /// Get the element of the property array.
    virtual void            Get(const Serializable *ptr, int index, Variant &dest) const = 0;

                            /// Set the property.
    virtual void            Set(Serializable *ptr, const Variant &src) = 0;

                            /// Set the element of the property array.
    virtual void            Set(Serializable *ptr, int index, const Variant &src) = 0;

                            /// Get the count of the property array.
    virtual int             GetCount(const Serializable *ptr) const = 0;

                            /// Set the count of the property array.
    virtual void            SetCount(Serializable *ptr, int count) = 0;
};

class BE_API PropertyInfo {
    friend class Serializable;

    using PropertyAccessorPtr = std::shared_ptr<PropertyAccessor>;

public:
    /// Property flags
    struct Flag {
        enum Enum {
            Empty               = 0,
            ReadOnly            = BIT(1),   ///< Don't allow to set
            SkipSerialization   = BIT(2),   ///< Don't serialize
            Editor              = BIT(3),   ///< Show in editor
            NonCopying          = BIT(4),   ///< Don't allow to copy
            SystemUnits         = BIT(5),
            MultiLines          = BIT(6),   ///< Str type in multilines
            Array               = BIT(7),   ///< Is array property ?
            Network             = BIT(8),   ///< Not used yet
            ShaderDefine        = BIT(9),
            ForceToSet          = BIT(10),  ///< Internal use
        };
    };

    PropertyInfo();
    PropertyInfo(const char *name, const char *label, Variant::Type::Enum type, intptr_t offset, const Variant &defaultValue, const char *desc, int flags);
    PropertyInfo(const char *name, const char *label, Variant::Type::Enum type, PropertyAccessor *accessor, const Variant &defaultValue, const char *desc, int flags);

    Variant::Type::Enum     GetType() const { return type; }
    const char *            GetName() const { return name; }
    const Variant &         GetDefaultValue() const { return defaultValue; }
    const char *            GetLabel() const { return label; }
    const char *            GetDescription() const { return desc; }
    int                     GetFlags() const { return flags; }

                            /// Tests if this property is array.
    bool                    IsArray() const { return !!(flags & Flag::Array); }
                            /// Tests if this property is read only.
    bool                    IsReadOnly() const { return !!(flags & Flag::ReadOnly); }
                            /// Tests if this property has bounded range.
    bool                    IsRanged() const { return range.IsValid(); }

                            /// Returns minimum value allowed.
    float                   GetMinValue() const { return range.minValue; }
                            /// Returns maximum value allowed.
    float                   GetMaxValue() const { return range.maxValue; }
                            /// Returns stepping value in editor.
    float                   GetStep() const { return range.step; }

                            /// Returns enumerator string array.
    const Array<Str> &      GetEnum() const { return enumeration; }

    const MetaObject *      GetMetaObject() const { return metaObject; }

    void                    SetRange(float minValue, float maxValue, float step) { range = Rangef(minValue, maxValue, step); }

    void                    SetEnumString(const Str &enumString) { SplitStringIntoList(enumeration, enumString, ";"); }

    void                    SetMetaObject(const MetaObject *metaObject) { this->metaObject = metaObject; }

    friend bool             ParseShaderPropertyInfo(Lexer &lexer, PropertyInfo &propInfo);

private:
    Variant::Type::Enum     type;               ///< Property type
    Variant                 defaultValue;       ///< Default value
    Str                     name;               ///< Property name
    Str                     label;              ///< Label in Editor
    Str                     desc;               ///< Description in Editor
    intptr_t                offset;             ///< Byte offsets from start of object
    PropertyAccessorPtr     accessor;
    Rangef                  range;
    Array<Str>              enumeration;        ///< Enumeration string list for enumeration type
    const MetaObject *      metaObject;         ///< MetaObject pointer for object type
    int                     flags;
};

BE_INLINE PropertyInfo::PropertyInfo() :
    type(Variant::Type::None),
    offset(0),
    metaObject(nullptr),
    flags(Flag::Empty) {
}

BE_INLINE PropertyInfo::PropertyInfo(const char *_name, const char *_label, Variant::Type::Enum _type, intptr_t _offset, const Variant &_defaultValue, const char *_desc, int _flags) :
    type(_type),
    name(_name),
    label(_label),
    defaultValue(_defaultValue),
    offset(_offset),
    desc(_desc),
    metaObject(nullptr),
    flags(_flags) {
}

BE_INLINE PropertyInfo::PropertyInfo(const char *_name, const char *_label, Variant::Type::Enum _type, PropertyAccessor *_accessor, const Variant &_defaultValue, const char *_desc, int _flags) :
    type(_type),
    name(_name),
    label(_label),
    defaultValue(_defaultValue),
    offset(0),
    accessor(_accessor),
    desc(_desc),
    metaObject(nullptr),
    flags(_flags) {
}

/// Property type trait (default use const reference for object type).
template <typename T, typename = void>
struct PropertyTrait {
    /// Get function return type.
    using ReturnType = const T &;
    /// Set function parameter type.
    using ParameterType = const T &;
};

/// Enum property trait.
template <typename T>
struct PropertyTrait<T, typename std::enable_if_t<std::is_enum<T>::value>> {
    using ReturnType = T;
    using ParameterType = T;
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

/// Double property trait.
template <>
struct PropertyTrait<double> {
    using ReturnType = double;
    using ParameterType = double;
};

/// Mixed property trait (use const reference for set function only).
template <typename T>
struct MixedPropertyTrait {
    using ReturnType = T;
    using ParameterType = const T &;
};

/// Template implementation of the property accessor.
template <typename Class, typename Type, template <typename T, typename...> class Trait = PropertyTrait>
class PropertyAccessorImpl : public PropertyAccessor {
public:
    using GetFunctionPtr = typename Trait<Type>::ReturnType(Class::*)() const;
    using SetFunctionPtr = void (Class::*)(typename Trait<Type>::ParameterType);

    /// Class-specific pointer to getter function.
    GetFunctionPtr getFunction;
    /// Class-specific pointer to setter function.
    SetFunctionPtr setFunction;

    /// Construct with function pointers.
    PropertyAccessorImpl(GetFunctionPtr getter, SetFunctionPtr setter) :
        getFunction(getter), setFunction(setter) {
        assert(getFunction);
        assert(setFunction);
    }

    /// Invoke getter function.
    virtual void Get(const Serializable *ptr, Variant &out) const override {
        assert(ptr);
        const Class *classPtr = static_cast<const Class *>(ptr);
        out = (classPtr->*getFunction)();
    }

    /// Get the element of the property array.
    virtual void Get(const Serializable *ptr, int index, Variant &dest) const override {
        assert(0);
    }

    /// Invoke setter function.
    virtual void Set(Serializable *ptr, const Variant &in) override {
        assert(ptr);
        Class *classPtr = static_cast<Class *>(ptr);
        (classPtr->*setFunction)(in.As<Type>());
    }

    /// Set the element of the property array.
    virtual void Set(Serializable *ptr, int index, const Variant &src) override {
        assert(0);
    }

    /// Get the count of the property array.
    virtual int GetCount(const Serializable *ptr) const override {
        assert(0);
        return 0;
    }

    /// Set the count of the property array.
    virtual void SetCount(Serializable *ptr, int count) override {
        assert(0);
    }
};

/// Template implementation of the property lambda accessor.
template <typename Class, typename Type, template <typename T, typename...> class Trait = PropertyTrait>
class PropertyLambdaAccessorImpl : public PropertyAccessor {
public:
    /// Class-specific std::function to getter lambda.
    std::function<typename Trait<Type>::ReturnType()> getFunction;
    /// Class-specific std::function to setter lambda.
    std::function<void(typename Trait<Type>::ParameterType)> setFunction;

    /// Construct with lambdas.
    template <typename Getter, typename Setter>
    PropertyLambdaAccessorImpl(Getter &&getter, Setter &&setter) :
        getFunction(std::forward<Getter>(getter)), setFunction(std::forward<Setter>(setter)) {
        assert(getFunction);
        assert(setFunction);
    }

    /// Invoke getter lambda.
    virtual void Get(const Serializable *ptr, Variant &out) const override {
        out = getFunction();
    }

    /// Get the element of the property array.
    virtual void Get(const Serializable *ptr, int index, Variant &dest) const override {
        assert(0);
    }

    /// Invoke setter lambda.
    virtual void Set(Serializable *ptr, const Variant &in) override {
        setFunction(in.As<Type>());
    }

    /// Set the element of the property array.
    virtual void Set(Serializable *ptr, int index, const Variant &src) override {
        assert(0);
    }

    /// Get the count of the property array.
    virtual int GetCount(const Serializable *ptr) const override {
        assert(0);
        return 0;
    }

    /// Set the count of the property array.
    virtual void SetCount(Serializable *ptr, int count) override {
        assert(0);
    }
};

/// Template implementation of the array property accessor.
template <typename Class, typename Type, template <typename T, typename...> class Trait = PropertyTrait>
class ArrayPropertyAccessorImpl : public PropertyAccessor {
public:
    using GetFunctionPtr = typename Trait<Type>::ReturnType(Class::*)(int index) const;
    using SetFunctionPtr = void (Class::*)(int index, typename Trait<Type>::ParameterType);
    using GetCountFunctionPtr = int(Class::*)() const;
    using SetCountFunctionPtr = void(Class::*)(int count);

    /// Construct with function pointers.
    ArrayPropertyAccessorImpl(GetFunctionPtr getter, SetFunctionPtr setter, GetCountFunctionPtr getCount, SetCountFunctionPtr setCount) :
        getFunction(getter), setFunction(setter), getCountFunction(getCount), setCountFunction(setCount) {
        assert(getFunction);
        assert(setFunction);
        assert(getCountFunction);
        assert(setCountFunction);
    }

    /// Invoke getter function.
    virtual void Get(const Serializable *ptr, Variant &out) const override {
        assert(0);
    }

    /// Invoke getter function.
    virtual void Get(const Serializable *ptr, int index, Variant &out) const override {
        assert(ptr);
        const Class *classPtr = static_cast<const Class *>(ptr);
        out = (classPtr->*getFunction)(index);
    }

    /// Invoke setter function.
    virtual void Set(Serializable *ptr, const Variant &src) override {
        assert(0);
    }

    /// Invoke setter function.
    virtual void Set(Serializable *ptr, int index, const Variant &in) override {
        assert(ptr);
        Class *classPtr = static_cast<Class *>(ptr);
        (classPtr->*setFunction)(index, in.As<Type>());
    }

    /// Invoke get count function.
    virtual int GetCount(const Serializable *ptr) const override {
        assert(ptr);
        const Class *classPtr = static_cast<const Class *>(ptr);
        return (classPtr->*getCountFunction)();
    }

    /// Invoke set count function.
    virtual void SetCount(Serializable *ptr, int count) override {
        assert(ptr);
        Class *classPtr = static_cast<Class *>(ptr);
        (classPtr->*setCountFunction)(count);
    }

    /// Class-specific pointer to getter function.
    GetFunctionPtr getFunction;
    /// Class-specific pointer to setter function.
    SetFunctionPtr setFunction;
    /// Class-specific pointer to get count function.
    GetCountFunctionPtr getCountFunction;
    /// Class-specific pointer to set count function.
    SetCountFunctionPtr setCountFunction;
};

#define UNREGISTER_PROPERTY(name) Class::metaObject.UnregisterProperty(name)

#define REGISTER_PROPERTY(name, label, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        ::offset_of(&Class::var), defaultValue, desc, flags))

#define REGISTER_ARRAY_PROPERTY(name, label, type, var, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        ::offset_of(&Class::var), defaultValue, desc, flags | BE1::PropertyInfo::Flag::Array))

#define REGISTER_ACCESSOR_PROPERTY(name, label, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter), defaultValue, desc, flags))

#define REGISTER_ACCESSOR_ARRAY_PROPERTY(name, label, type, getter, setter, getCount, setCount, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        new BE1::ArrayPropertyAccessorImpl<Class, type>(&Class::getter, &Class::setter, &Class::getCount, &Class::setCount), \
        defaultValue, desc, flags | BE1::PropertyInfo::Flag::Array))

#define REGISTER_MIXED_ACCESSOR_PROPERTY(name, label, type, getter, setter, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        new BE1::PropertyAccessorImpl<Class, type, BE1::MixedPropertyTrait>(&Class::getter, &Class::setter), \
        defaultValue, desc, flags))

#define REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY(name, label, type, getter, setter, getCount, setCount, defaultValue, desc, flags) \
    Class::metaObject.RegisterProperty(BE1::PropertyInfo(name, label, BE1::VariantType<type>::GetType(), \
        new BE1::ArrayPropertyAccessorImpl<Class, type, BE1::MixedPropertyTrait>(&Class::getter, &Class::setter, &Class::getCount, &Class::setCount), \
        defaultValue, desc, flags | BE1::PropertyInfo::Flag::Array))

BE_NAMESPACE_END
