// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Variant.h"
#include "Signal.h"

namespace Json {
    class Value;
}

BE_NAMESPACE_BEGIN

class PropertyDesc;

/// Interface for objects with automatic serialization through properties.
class BE_API Serializable : public SignalObject {
public:
                            /// Gets property descriptor by name. Returns false if not found.
    bool                    GetPropertyDesc(const char *name, PropertyDesc &outPropertyDesc) const;
                            /// Gets property descriptor by index. Returns false if not found.
    bool                    GetPropertyDesc(int index, PropertyDesc &outPropertyDesc) const;
                            /// Gets property descriptor array.
    virtual void            GetPropertyDescList(Array<PropertyDesc> &outPropertyDescList) const = 0;

                            /// Serialize to JSON value.
    virtual void            Serialize(Json::Value &out, bool forCopying = false) const;
                            /// Deserialize from JSON value.
    virtual void            Deserialize(const Json::Value &in);

                            /// Returns a property default value by name. Returns empty variant if not found.
    Variant                 GetPropertyDefault(const char *name) const;
                            /// Returns a property default value by index. Returns empty variant if not found.
    Variant                 GetPropertyDefault(int index) const;
    
                            /// Gets a property value by name. Returns empty variant if not found.
    Variant                 GetProperty(const char *name) const;
                            /// Gets a property value by index. Returns empty variant if not found.
    Variant                 GetProperty(int index) const;
                            /// Gets a property value by property descriptor.
    void                    GetProperty(const PropertyDesc &propertyDesc, Variant &out) const;

                            /// Sets a property value by name. Returns true if successfully set.
    bool                    SetProperty(const char *name, const Variant &value, bool forceWrite = false);
                            /// Sets a property value by index. Returns true if successfully set.
    bool                    SetProperty(int index, const Variant &value, bool forceWrite = false);
                            /// Sets a property value by property descriptor. Returns true if successfully set.
    bool                    SetProperty(const PropertyDesc &propertyDesc, const Variant &value, bool forceWrite = false);

                            /// Sets a property value to default by name. Returns true if successfully set.
    bool                    SetPropertyDefault(const char *name, bool forceWrite = false);
                            /// Sets a property value to default by index. Returns true if successfully set.
    bool                    SetPropertyDefault(int index, bool forceWrite = false);
    
                            /// Gets a indexed property value by name. Returns empty variant if not found.
    Variant                 GetArrayProperty(const char *name, int elementIndex) const;
                            /// Gets a indexed property value by index. Returns empty variant if not found.
    Variant                 GetArrayProperty(int index, int elementIndex) const;
                            /// Gets a indexed property value by property descriptor.
    void                    GetArrayProperty(const PropertyDesc &propertyDesc, int elementIndex, Variant &out) const;

                            /// Sets a indexed property value by name. Returns true if successfully set.
    bool                    SetArrayProperty(const char *name, int elementIndex, const Variant &value, bool forceWrite = false);
                            /// Sets a indexed property value by index. Returns true if successfully set.
    bool                    SetArrayProperty(int index, int elementIndex, const Variant &value, bool forceWrite = false);
                            /// Sets a indexed property value by property descriptor.
    bool                    SetArrayProperty(const PropertyDesc &propertyDesc, int elementIndex, const Variant &value, bool forceWrite = false);

                            /// Returns property array count by name. This function is valid only if property is an array.
    int                     GetPropertyArrayCount(const char *name) const;
                            /// Returns property array count by index. This function is valid only if property is an array.
    int                     GetPropertyArrayCount(int index) const;
                            /// Returns property array count by property descriptor. This function is valid only if property is an array.
    int                     GetPropertyArrayCount(const PropertyDesc &propertyDesc) const;

                            /// Sets property array count by name. This function is valid only if property is an array.
    void                    SetPropertyArrayCount(const char *name, int numElements);
                            /// Sets property array count by index. This function is valid only if property is an array.
    void                    SetPropertyArrayCount(int index, int numElements);
                            /// Sets property array count by property descriptor. This function is valid only if property is an array.
    void                    SetPropertyArrayCount(const PropertyDesc &propertyDesc, int numElements);

    static const SignalDef  SIG_PropertyChanged;            ///< A signal emitted when a property value changed.
    static const SignalDef  SIG_PropertyArrayCountChanged;  ///< A signal emitted when a property array count changed.
    static const SignalDef  SIG_PropertyDescUpdated;        ///< A signal emitted when property descriptor list updated.
};

BE_NAMESPACE_END
