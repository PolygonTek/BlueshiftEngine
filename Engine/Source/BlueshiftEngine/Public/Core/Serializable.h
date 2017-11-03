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

class PropertyInfo;

class Property {
    friend class Properties;

public:
    enum Flag {
        Hidden              = BIT(0)
    };

    Property() : numElements(0), flags(0) {}
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

    void                    Purge();

                            /// Returns property info by name.
    bool                    GetInfo(const char *name, PropertyInfo &propertyInfo) const;

                            /// Returns number of elements of array property
                            /// This function is valid only if property is an array
    int                     NumElements(const char *name) const;

                            /// Sets number of elements of array property
                            /// This function is valid only if property is an array
    void                    SetNumElements(const char *name, int numElements);

                            /// Gets default value
    bool                    GetDefaultValue(const char *name, Variant &out) const;

                            /// Returns default value
    Variant                 GetDefaultValue(const char *name) const;

                            /// Gets property
    bool                    Get(const char *name, Variant &out) const;

                            /// Gets property, if it is a invalid property, returns 0-set value or "" (empty string)
    Variant                 Get(const char *name) const;

                            /// Sets property
    bool                    Set(const char *name, const Variant &value, bool forceWrite = false);

                            /// Deserialize from JSON value
    void                    Deserialize(const Json::Value &in);

                            /// Serialize to JSON value
    void                    Serialize(Json::Value &out) const;

    static const SignalDef  SIG_PropertyChanged;
    static const SignalDef  SIG_PropertyArrayNumChanged;
    static const SignalDef  SIG_PropertyUpdated;

protected:
    Object *                owner;

#ifndef NEW_PROPERTY_SYSTEM
    StrHashMap<Property>    propertyHashMap;
#endif
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

BE_NAMESPACE_END