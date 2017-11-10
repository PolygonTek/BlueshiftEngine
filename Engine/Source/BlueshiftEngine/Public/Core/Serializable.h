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
#include "Variant.h"
#include "Signal.h"

BE_NAMESPACE_BEGIN

class PropertyInfo;

class BE_API Serializable : public SignalObject {
public:
                            /// Gets property info with the given property name.
    bool                    GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const;

                            /// Gets property info with the given property index.
    bool                    GetPropertyInfo(int index, PropertyInfo &propertyInfo) const;

                            /// Gets property info array.
    virtual void            GetPropertyInfoList(Array<PropertyInfo> &propertyInfos) const = 0;

                            /// Serialize to JSON value.
    void                    Serialize(Json::Value &out) const;

                            /// Deserialize from JSON value.
    void                    Deserialize(const Json::Value &in);

                            /// Gets a property value by name. Returns empty variant if not found.
    Variant                 GetProperty(const char *name) const;

                            /// Gets a indexed property value by name. Returns empty variant if not found.
    Variant                 GetProperty(const char *name, int index) const;

                            /// Returns a property default value by name. Returns empty variant if not found.
    Variant                 GetPropertyDefault(const char *name) const;

                            /// Sets a property value by name. Returns true if successfully set.
    bool                    SetProperty(const char *name, const Variant &value);

                            /// Sets a indexed property value. Returns true if successfully set.
    bool                    SetProperty(const char *name, int index, const Variant &value);

                            /// Returns number of elements of array property.
                            /// This function is valid only if property is an array.
    int                     GetPropertyArrayCount(const char *name) const;

                            /// Sets number of elements of array property.
                            /// This function is valid only if property is an array.
    void                    SetPropertyArrayCount(const char *name, int numElements);

    static const SignalDef  SIG_PropertyChanged;
    static const SignalDef  SIG_PropertyArrayCountChanged;
    static const SignalDef  SIG_PropertyUpdated;

private:
    bool                    SetPropertyWithoutSignal(const char *name, const Variant &value);
    bool                    SetPropertyWithoutSignal(const char *name, int index, const Variant &value);
};

BE_NAMESPACE_END
