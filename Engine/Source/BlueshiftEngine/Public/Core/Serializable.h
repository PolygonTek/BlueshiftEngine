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

//-------------------------------------------------------------------------------
//
// Serializable
//
//-------------------------------------------------------------------------------

class Serializable : public SignalObject {
public:
    explicit Serializable(Object *owner);

                            /// Serialize to JSON value
    void                    Serialize(Json::Value &out) const;

                            /// Deserialize from JSON value
    void                    Deserialize(const Json::Value &in);

                            /// Returns a default value
    Variant                 GetPropertyDefault(const char *name) const;

                            /// Gets a property value
    Variant                 GetProperty(const char *name) const;

                            /// Gets a indexed property value
    Variant                 GetProperty(const char *name, int index) const;

                            /// Sets a property value
    bool                    SetProperty(const char *name, const Variant &value);

                            /// Sets a indexed property value
    bool                    SetProperty(const char *name, int index, const Variant &value);

                            /// Returns number of elements of array property
                            /// This function is valid only if property is an array
    int                     GetPropertyArrayCount(const char *name) const;

                            /// Sets number of elements of array property
                            /// This function is valid only if property is an array
    void                    SetPropertyArrayCount(const char *name, int numElements);

    static const SignalDef  SIG_PropertyChanged;
    static const SignalDef  SIG_PropertyArrayCountChanged;
    static const SignalDef  SIG_PropertyUpdated;

private:
    bool                    SetPropertyWithoutSignal(const char *name, const Variant &value);
    bool                    SetPropertyWithoutSignal(const char *name, int index, const Variant &value);

    Object *                owner;
};

BE_NAMESPACE_END
