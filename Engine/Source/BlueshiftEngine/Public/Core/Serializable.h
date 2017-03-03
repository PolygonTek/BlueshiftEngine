#pragma once

#include "jsoncpp/include/json/json.h"
#include "Signal.h"
#include "Core/Variant.h"
#include "Containers/HashMap.h"

BE_NAMESPACE_BEGIN

class Serializable {
public:
};

//-------------------------------------------------------------------------------
//
// Properties
//
//-------------------------------------------------------------------------------

class PropertySpec;

class Property {
    friend class Properties;

public:
    enum Flag {
        Hidden              = BIT(0)
    };

    Property() : numElements(0), flags(0) { value.SetEmpty(); }
    Property(const Variant &_value, int _flags) : value(_value), numElements(0), flags(_flags) {}

    const Variant &         Value() const { return value; }
    Variant &               Value() { return value; }

    int                     Flags() const { return flags; }
       
private:
    Variant                 value;
    int                     numElements;
    int                     flags;
};

class Properties {
public:
    explicit Properties(Object *owner);
    ~Properties();

    void                    Init(const Properties *props);
    void                    Init(const Json::Value &node);

    void                    Purge();

                            /// Returns number of properties.
    int                     Count() const { return propertyHashMap.Count(); }

                            /// Returns property name by index.
    const char *            GetName(int index) const;

                            /// Returns property spec by index.
    const PropertySpec *    GetSpec(int index) const;

                            /// Returns property spec by spec name.
    const PropertySpec *    GetSpec(const char *name) const;

                            /// Returns number of elements of array property
                            /// This function is valid only if property is an array
    int                     NumElements(const char *name) const;

                            /// Sets number of elements of array property
                            /// This function is valid only if property is an array
    void                    SetNumElements(const char *name, int numElements);

                            /// Returns property flag
    int                     GetFlags(const char *name) const;

                            /// Sets property flag
    void                    SetFlags(const char *name, int flags);

                            /// Gets default value
    bool                    GetDefaultValue(const char *name, Variant &out) const;

                            /// Returns default value
    Variant                 GetDefaultValue(const char *name) const;

                            /// Gets property
    bool                    Get(const char *name, Variant &out, bool forceRead = false) const;

                            /// Gets property, if it is a invalid property, returns 0-set value or "" (empty string)
    Variant                 Get(const char *name) const;

                            /// Gets property with vargs (name1, variant_ptr1, name2, variant_ptr2, ...)
    bool                    GetVa(const char *name, ...) const;

                            /// Sets property
    bool                    Set(const char *name, const Variant &value, bool forceWrite = false);

                            /// Sets property with vargs (name1, variant_ptr1, name2, variant_ptr2, ...)
    bool                    SetVa(const char *name, ...);
        
                            /// Deserialize to Json::Value
    const Json::Value       Deserialize() const;

                            /// Serialize to JSON node
    void                    Serialize(Json::Value &out) const;

protected:
    Object *                owner;
    StrHashMap<Property>    propertyHashMap; //
};

BE_INLINE Variant Properties::GetDefaultValue(const char *name) const {
    Variant out;
    GetDefaultValue(name, out);
    return out;
}

BE_INLINE Variant Properties::Get(const char *name) const {
    Variant out;
    Get(name, out);
    return out;
}

extern const SignalDef      SIG_PropertyChanged;
extern const SignalDef      SIG_PropertyArrayNumChanged;
extern const SignalDef      SIG_PropertyFlagsChanged;

BE_NAMESPACE_END