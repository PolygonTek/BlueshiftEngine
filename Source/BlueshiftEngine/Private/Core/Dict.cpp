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
#include "Core/Checksum_CRC32.h"
#include "Core/Dict.h"

BE_NAMESPACE_BEGIN

StrPool Dict::globalKeys;
StrPool Dict::globalValues;

const KeyValue *Dict::MatchPrefix(const char *prefix, const KeyValue *lastMatch) const {
    assert(prefix);
    int len = (int)strlen(prefix);

    int start = -1;
    if (lastMatch) {
        start = kvArray.FindIndex(*lastMatch);
        assert(start >= 0);
        if (start < 1) {
            start = 0;
        }
    }

    for (int i = start + 1; i < kvArray.Count(); i++) {
        if (!kvArray[i].GetKey().Icmpn(prefix, len)) {
            return &kvArray[i];
        }
    }
    return nullptr;
}

Dict &Dict::operator=(const Dict &other) {
    // check for assignment to self
    if (this == &other) {
        return *this;
    }
    
    Clear();

    kvArray = other.kvArray;
    kvHash = other.kvHash;

    for (int i = 0; i < kvArray.Count(); i++) {
        kvArray[i].key = keyPool->CopyString(kvArray[i].key);
        kvArray[i].value = valuePool->CopyString(kvArray[i].value);
    }

    return *this;
}

void Dict::Copy(const Dict &other) {
    // check for assignment to self
    if (this == &other) {
        return;
    }

    int n = other.kvArray.Count();

    int *found;
    if (kvArray.Count()) {
        found = (int *)_alloca16(other.kvArray.Count() * sizeof(int));
        for (int i = 0; i < n; i++) {
            found[i] = FindKeyValueIndex(other.kvArray[i].GetKey());
        }
    } else {
        found = nullptr;
    }

    for (int i = 0; i < n; i++) {
        if (found && found[i] != -1) {
            // first set the new value and then free the old value to allow proper self copying
            const PoolStr *oldValue = kvArray[found[i]].value;
            kvArray[found[i]].value = valuePool->CopyString(other.kvArray[i].value);
            valuePool->ReleaseString(oldValue);
        } else {
            KeyValue kv;
            kv.key = keyPool->CopyString(other.kvArray[i].key);
            kv.value = valuePool->CopyString(other.kvArray[i].value);
            kvHash.Add(kvHash.GenerateHash(kv.GetKey(), false), kvArray.Append(kv));
        }
    }
}

void Dict::TransferKeyValues(Dict &other) {
    if (this == &other) {
        return;
    }
    
    if (other.kvArray.Count() && other.kvArray[0].key->GetPool() != keyPool) {
        BE_FATALERROR(L"Dict::TransferKeyValues: can't transfer values across a DLL boundary");
        return;
    }

    Clear();

    int n = other.kvArray.Count();
    kvArray.SetCount(n);
    for (int i = 0; i < n; i++) {
        // 같은 key/value pool 을 사용하고 있으므로 주소만 복사한다.
        kvArray[i].key = other.kvArray[i].key;
        kvArray[i].value = other.kvArray[i].value;
    }
    kvHash = other.kvHash;

    // other 는 clear
    other.kvArray.Clear();
    other.kvHash.Free();
}

void Dict::SetDefaults(const Dict *dict) {
    int n = dict->kvArray.Count();
    for (int i = 0; i < n; i++) {
        const KeyValue *def = &dict->kvArray[i];
        const KeyValue *kv = FindKeyValue(def->GetKey());
        if (!kv) {
            KeyValue newkv;
            newkv.key = keyPool->CopyString(def->key);
            newkv.value = valuePool->CopyString(def->value);
            kvHash.Add(kvHash.GenerateHash(newkv.GetKey(), false), kvArray.Append(newkv));
        }
    }
}

int	Dict::Checksum() const {
    uint32_t ret;
    Array<KeyValue> sorted = kvArray;
    auto compareFunc = [](const KeyValue &arg1, const KeyValue &arg2) -> bool {
        return Str::Cmp(arg1.GetKey(), arg2.GetKey()) < 0 ? true : false;
    };
    sorted.Sort(compareFunc);

    int n = sorted.Count();
    CRC32_InitChecksum(ret);
    for (int i = 0; i < n; i++) {
        CRC32_UpdateChecksum(ret, sorted[i].GetKey().c_str(), sorted[i].GetKey().Length());
        CRC32_UpdateChecksum(ret, sorted[i].GetValue().c_str(), sorted[i].GetValue().Length());
    }
    CRC32_FinishChecksum(ret);
    return ret;
}

const KeyValue *Dict::FindKeyValue(const char *key) const {
    if (key == nullptr || key[0] == '\0') {
        BE_DLOG(L"Dict::FindKeyValue: empty key");
        return nullptr;
    }

    int hash = kvHash.GenerateHash(key, false);
    for (int i = kvHash.First(hash); i != -1; i = kvHash.Next(i)) {
        if (kvArray[i].GetKey().Icmp(key) == 0) {
            return &kvArray[i];
        }
    }

    return nullptr;
}

int Dict::FindKeyValueIndex(const char *key) const {
    if (key == nullptr || key[0] == '\0') {
        BE_WARNLOG(L"Dict::FindKeyValueIndex: empty key");
        return 0;
    }

    int hash = kvHash.GenerateHash(key, false);
    for (int i = kvHash.First(hash); i != -1; i = kvHash.Next(i)) {
        if (kvArray[i].GetKey().Icmp(key) == 0) {
            return i;
        }
    }

    return -1;
}

bool Dict::GetString(const char *key, const char *defaultString, const char **out) const {
    const KeyValue *kv = FindKeyValue(key);
    if (kv) {
        *out = kv->GetValue();
        return true;
    }

    *out = defaultString;
    return false;
}

bool Dict::GetString(const char *key, const char *defaultString, Str &out) const {
    const KeyValue *kv = FindKeyValue(key);
    if (kv) {
        out = kv->GetValue();
        return true;
    }
    out = defaultString;
    return false;
}

const char *Dict::GetString(const char *key, const char *defaultString) const {
    const KeyValue *kv = FindKeyValue(key);
    if (kv) {
        return kv->GetValue();
    }
    return defaultString;
}

bool Dict::GetFloat(const char *key, const char *defaultString, float &out) const {
    const char *s;
    bool found = GetString(key, defaultString, &s);
    out = (float)atof(s);
    return found;
}

bool Dict::GetInt(const char *key, const char *defaultString, int &out) const {
    const char *s;
    bool found = GetString(key, defaultString, &s);
    out = atoi(s);
    return found;
}

bool Dict::GetInt64(const char *key, const char *defaultString, int64_t &out) const {
    const char *s;
    bool found = GetString(key, defaultString, &s);
    sscanf(s, "%" PRIi64, &out);
    return found;
}

bool Dict::GetBool(const char *key, const char *defaultString, bool &out) const {
    const char *s;
    bool found = GetString(key, defaultString, &s);
    out = (atoi(s) != 0);
    return found;
}

bool Dict::GetPoint(const char *key, const char *defaultString, Point &out) const {
    if (!defaultString) {
        defaultString = "0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.x = 0;
    out.y = 0;
    sscanf(s, "%i %i", &out.x, &out.y);
    return found;
}

bool Dict::GetRect(const char *key, const char *defaultString, Rect &out) const {
    if (!defaultString) {
        defaultString = "0 0 0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.Set(0, 0, 0, 0);
    sscanf(s, "%i %i %i %i", &out.x, &out.y, &out.w, &out.h);
    return found;
}

bool Dict::GetAngles(const char *key, const char *defaultString, Angles &out) const {
    if (!defaultString) {
        defaultString = "0 0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.SetZero();	
    sscanf(s, "%f %f %f", &out.yaw, &out.pitch, &out.roll);
    return found;
}

bool Dict::GetVec3(const char *key, const char *defaultString, Vec3 &out) const {
    if (!defaultString) {
        defaultString = "0 0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.SetFromScalar(0);
    sscanf(s, "%f %f %f", &out.x, &out.y, &out.z);
    return found;
}

bool Dict::GetVec2(const char *key, const char *defaultString, Vec2 &out) const {	
    if (!defaultString) {
        defaultString = "0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.SetFromScalar(0);
    sscanf(s, "%f %f", &out.x, &out.y);
    return found;
}

bool Dict::GetVec4(const char *key, const char *defaultString, Vec4 &out) const {
    if (!defaultString) {
        defaultString = "0 0 0 0";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.SetFromScalar(0);
    sscanf(s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w);
    return found;
}

bool Dict::GetMatrix(const char *key, const char *defaultString, Mat3 &out) const {
    if (!defaultString) {
        defaultString = "1 0 0 0 1 0 0 0 1";
    }

    const char *s;
    bool found = GetString(key, defaultString, &s);
    out.SetIdentity();
    sscanf(s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z);
    return found;
}

void Dict::Set(const char *key, const char *value) {
    if (key == nullptr || key[0] == '\0') {
        return;
    }

    int i = FindKeyValueIndex(key);
    if (i != -1) {
        // first set the new value and then free the old value to allow proper self copying
        const PoolStr *oldValue = kvArray[i].value;
        kvArray[i].value = valuePool->AllocString(value);
        valuePool->ReleaseString(oldValue);
    } else {
        KeyValue kv;
        kv.key = keyPool->AllocString(key);
        kv.value = valuePool->AllocString(value);
        kvHash.Add(kvHash.GenerateHash(kv.GetKey(), false), kvArray.Append(kv));
    }
}

bool Dict::Parse(Lexer &parser) {
    Str token;
    Str token2;

    bool errors = false;

    if (!parser.ExpectTokenString("{")) {
        return false;
    }

    parser.ReadToken(&token);

    while ((parser.GetTokenType() != TokenType::TT_PUNCTUATION) || (token != "}")) {
        if (parser.GetTokenType() != TokenType::TT_STRING) {
            parser.Error("Expected quoted string, but found '%s'", token.c_str());
        }

        if (!parser.ReadToken(&token2)) {
            parser.Error("Unexpected end of file");
        }

        if (FindKeyValue(token)) {
            parser.Warning("'%s' already defined", token.c_str());
            errors = true;
        }

        Set(token, token2);

        if (!parser.ReadToken(&token)) {
            parser.Error("Unexpected end of file");
        }
    }

    return !errors;
}

void Dict::Delete(const char *key) {
    int hash = kvHash.GenerateHash(key, false);
    for (int i = kvHash.First(hash); i != -1; i = kvHash.Next(i)) {
        if (kvArray[i].GetKey().Icmp(key) == 0) {
            keyPool->ReleaseString(kvArray[i].key);
            valuePool->ReleaseString(kvArray[i].value);
            kvArray.RemoveIndex(i);
            kvHash.RemoveIndex(hash, i);
            break;
        }
    }

#if 0
    // make sure all keys can still be found in the hash index
    for (i = 0; i < kvArray.Count(); i++) {
        assert(FindKeyValue(kvArray[i].GetKey()) != nullptr);
    }
#endif
}

void Dict::Clear() {
    for (int i = 0; i < kvArray.Count(); i++) {
        keyPool->ReleaseString(kvArray[i].key);
        valuePool->ReleaseString(kvArray[i].value);
    }

    kvArray.Clear();
    kvHash.Free();
}

void Dict::Print() const {
    int n = kvArray.Count();
    for (int i = 0; i < n; i++) {
        BE_LOG(L"%hs = %hs\n", kvArray[i].GetKey().c_str(), kvArray[i].GetValue().c_str());
    }
}

BE_NAMESPACE_END
