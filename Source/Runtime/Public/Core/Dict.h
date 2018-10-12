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

#include "Containers/StrPool.h"
#include "Lexer.h"
#include "Math/Math.h"
#include "Math/Matrix.h"
#include "Math/Rect.h"
#include "Math/Angles.h"

BE_NAMESPACE_BEGIN

/// A Key-Value class for Dict class
class BE_API KeyValue {
    friend class Dict;

public:
                        /// Returns total size of allocated memory.
    size_t              Allocated() const { return key->Allocated() + value->Allocated(); }

                        /// Returns total size of allocated memory including size of this type.
    size_t              Size() const { return sizeof(*this) + key->Size() + value->Size(); }

                        /// Compare with another key-value.
    bool                operator==(const KeyValue &rhs) const { return (key == rhs.key && value == rhs.value); }

                        /// Returns a key string.
    const Str &         GetKey() const { return *key; }
                        /// Returns a value string.
    const Str &         GetValue() const { return *value; }

private:
    const PoolStr *     key;
    const PoolStr *     value;
};

/// A Key-Value dictionary
class BE_API Dict {
public:
    Dict();
    /// Copy constructor
    Dict(const Dict &other);
    /// Clears existing key-value pairs and copy all key-value pairs from other
    Dict &operator=(const Dict &other);
    /// Destructor
    ~Dict();

                        /// Returns total size of allocated memory
    size_t              Allocated() const;

                        /// Returns total size of allocated memory including size of this type
    size_t              Size() const { return sizeof(*this) + Allocated(); }

                        /// Returns number of key-values
    int                 NumKeyValues() const { return kvArray.Count(); }

                        /// Returns key-value with the given index.
                        /// Returns nullptr if the index is not valid range.
    const KeyValue *    GetKeyValue(int index) const;

                        /// Returns index of the key-value with the given key string
                        /// Returns -1 if no key matched.
    int                 FindKeyValueIndex(const char *key) const;

                        /// Returns key-value with the given key string.
                        /// Returns nullptr if no key matched.
    const KeyValue *    FindKeyValue(const char *key) const;

                        // prefix string 으로 match 되는 key-value 를 찾는다. 옵션으로 lastMatch 이후부터 찾을 수 있다.
    const KeyValue *    MatchPrefix(const char *prefix, const KeyValue *lastMatch = nullptr) const;
    
                        /// Copys all key-value pairs without removing existing key-value pairs not present in the other dict
    void                Copy(const Dict &other);

                        /// Clears existing key-value pairs and transfer key-value pairs from other
    void                TransferKeyValues(Dict &other);

                        // 존재하지 않는 key-value 들을 dict 로부터 copy
    void                SetDefaults(const Dict *dict);

                        // these return default values of 0.0, 0 and false
    const char *        GetString(const char *key, const char *defaultString = "") const;
    float               GetFloat(const char *key, const char *defaultString = "0") const;
    int                 GetInt(const char *key, const char *defaultString = "0") const;
    int64_t             GetInt64(const char *key, const char *defaultString = "0") const;
    bool                GetBool(const char *key, const char *defaultString = "false") const;
    Point               GetPoint(const char *key, const char *defaultString = nullptr) const;
    Rect                GetRect(const char *key, const char *defaultString = nullptr) const;
    Vec3                GetVec3(const char *key, const char *defaultString = nullptr) const;
    Vec2                GetVec2(const char *key, const char *defaultString = nullptr) const;
    Vec4                GetVec4(const char *key, const char *defaultString = nullptr) const;
    Angles              GetAngles(const char *key, const char *defaultString = nullptr) const;
    Mat3                GetMatrix(const char *key, const char *defaultString = nullptr) const;

    bool                GetString(const char *key, const char *defaultString, const char **out) const;
    bool                GetString(const char *key, const char *defaultString, Str &out) const;
    bool                GetFloat(const char *key, const char *defaultString, float &out) const;
    bool                GetInt(const char *key, const char *defaultString, int &out) const;
    bool                GetInt64(const char *key, const char *defaultString, int64_t &out) const;
    bool                GetBool(const char *key, const char *defaultString, bool &out) const;
    bool                GetPoint(const char *key, const char *defaultString, Point &out) const;
    bool                GetRect(const char *key, const char *defaultString, Rect &out) const;	
    bool                GetVec3(const char *key, const char *defaultString, Vec3 &out) const;
    bool                GetVec2(const char *key, const char *defaultString, Vec2 &out) const;
    bool                GetVec4(const char *key, const char *defaultString, Vec4 &out) const;
    bool                GetAngles(const char *key, const char *defaultString, Angles &out) const;
    bool                GetMatrix(const char *key, const char *defaultString, Mat3 &out) const;	

    void                Set(const char *key, const char *value);
    void                SetFloat(const char *key, float val);
    void                SetInt(const char *key, int val);
    void                SetInt64(const char *key, int64_t val);
    void                SetBool(const char *key, bool val);
    void                SetPoint(const char *key, const Point &val);
    void                SetRect(const char *key, const Rect &val);
    void                SetVec3(const char *key, const Vec3 &val);
    void                SetVec2(const char *key, const Vec2 &val);
    void                SetVec4(const char *key, const Vec4 &val);
    void                SetAngles(const char *key, const Angles &val);
    void                SetMatrix(const char *key, const Mat3 &val);

                        /// Parses dict from the parser
    bool                Parse(Lexer &parser);
    
                        /// Deletes a key-value with the given matching key.
    void                Delete(const char *key);

                        /// Clears all the key-values
    void                Clear();

                        /// Sets the granularity for the index
    void                SetGranularity(int granularity);

                        /// Sets hash size
    void                SetHashSize(int hashSize);
    
                        /// Returns a unique checksum for this dictionary's content
    int                 Checksum() const;

                        /// Prints the contents to the console
    void                Print() const;

protected:
    Array<KeyValue>     kvArray;        ///< key-value array
    HashIndex           kvHash;         ///< key-value array index hash
    StrPool *           keyPool;        ///< key string pool pointer
    StrPool *           valuePool;      ///< value string pool pointer
    static StrPool      globalKeys;     ///< default key string pool
    static StrPool      globalValues;   ///< default value string pool
};

BE_INLINE Dict::Dict() {
    keyPool = &globalKeys;
    valuePool = &globalValues;
    kvArray.SetGranularity(16);
    kvHash.SetGranularity(16);
    kvHash.Clear(128, 16);
}

BE_INLINE Dict::Dict(const Dict &other) {
    keyPool = &globalKeys;
    valuePool = &globalValues;
    *this = other;
}

BE_INLINE Dict::~Dict() {
    Clear();
}

BE_INLINE size_t Dict::Allocated() const {
    size_t size = kvArray.Allocated() + kvHash.Allocated();
    for (int i = 0; i < kvArray.Count(); i++) {
        size += kvArray[i].Size();
    }

    return size;
}

BE_INLINE const KeyValue *Dict::GetKeyValue(int index) const {
    if (index >= 0 && index < kvArray.Count()) {
        return &kvArray[index];
    }
    return nullptr;
}

BE_INLINE void Dict::SetGranularity(int granularity) {
    kvArray.SetGranularity(granularity);
    kvHash.SetGranularity(granularity);
}

BE_INLINE void Dict::SetHashSize(int hashSize) {
    if (kvArray.Count() == 0) {
        kvHash.Clear(hashSize, 16);
    }
}

BE_INLINE float Dict::GetFloat(const char *key, const char *defaultString) const {
    return (float)atof(GetString(key, defaultString));
}

BE_INLINE int Dict::GetInt(const char *key, const char *defaultString) const {
    return atoi(GetString(key, defaultString));
}

BE_INLINE int64_t Dict::GetInt64(const char *key, const char *defaultString) const {
    int64_t v;
    sscanf(GetString(key, defaultString), "%" PRIi64, &v);
    return v;
}

BE_INLINE bool Dict::GetBool(const char *key, const char *defaultString) const {
    return !Str::Cmp(GetString(key, defaultString), "true");
}

BE_INLINE Point Dict::GetPoint(const char *key, const char *defaultString) const {
    Point out;
    GetPoint(key, defaultString, out);
    return out;
}

BE_INLINE Rect Dict::GetRect(const char *key, const char *defaultString) const {
    Rect out;
    GetRect(key, defaultString, out);
    return out;
}

BE_INLINE Vec2 Dict::GetVec2(const char *key, const char *defaultString) const {
    Vec2 out;
    GetVec2(key, defaultString, out);
    return out;
}

BE_INLINE Vec3 Dict::GetVec3(const char *key, const char *defaultString) const {
    Vec3 out;
    GetVec3(key, defaultString, out);
    return out;
}

BE_INLINE Vec4 Dict::GetVec4(const char *key, const char *defaultString) const {
    Vec4 out;
    GetVec4(key, defaultString, out);
    return out;
}

BE_INLINE Angles Dict::GetAngles(const char *key, const char *defaultString) const {
    Angles out;
    GetAngles(key, defaultString, out);
    return out;
}

BE_INLINE Mat3 Dict::GetMatrix(const char *key, const char *defaultString) const {
    Mat3 out;
    GetMatrix(key, defaultString, out);
    return out;
}

BE_INLINE void Dict::SetFloat(const char *key, float val) {
    Set(key, va("%f", val));
}

BE_INLINE void Dict::SetInt(const char *key, int val) {
    Set(key, va("%i", val));
}

BE_INLINE void Dict::SetInt64(const char *key, int64_t val) {
    Set(key, va("%" PRIi64, val));
}

BE_INLINE void Dict::SetBool(const char *key, bool val) {
    Set(key, va("%i", val));
}

BE_INLINE void Dict::SetPoint(const char *key, const Point &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetRect(const char *key, const Rect &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetVec3(const char *key, const Vec3 &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetVec4(const char *key, const Vec4 &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetVec2(const char *key, const Vec2 &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetAngles(const char *key, const Angles &val) {
    Set(key, val.ToString());
}

BE_INLINE void Dict::SetMatrix(const char *key, const Mat3 &val) {
    Set(key, val.ToString());
}

BE_NAMESPACE_END
