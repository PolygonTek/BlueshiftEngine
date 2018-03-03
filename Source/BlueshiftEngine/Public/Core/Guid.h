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

BE_NAMESPACE_BEGIN

/// A GUID class
class BE_API Guid {
public:
    enum Format {
        Digits,                             ///< 00000000000000000000000000000000
        DigitsWithHyphens,                  ///< 00000000-0000-0000-0000-000000000000
        DigitsWithHyphensInBraces,          ///< {00000000-0000-0000-0000-000000000000}
        DigitsWithHyphensInParentheses,     ///< (00000000-0000-0000-0000-000000000000)
    };

    Guid() : u1(0), u2(0), u3(0), u4(0) { }
    Guid(uint32_t a, uint32_t b, uint32_t c, uint32_t d) : u1(a), u2(b), u3(c), u4(d) {}

    friend bool         operator==(const Guid &lhs, const Guid &rhs);
    friend bool         operator!=(const Guid &lhs, const Guid &rhs);

    friend bool         operator<(const Guid &lhs, const Guid &rhs);
    friend bool         operator>(const Guid &lhs, const Guid &rhs);

    uint32_t            operator[](int index) const;
    uint32_t &          operator[](int index);

    bool                IsZero() const { return (u1 | u2 | u3 | u4) == 0; }

    void                Set(uint32_t a, uint32_t b, uint32_t c, uint32_t d) { u1 = a; u2 = b; u3 = c; u4 = d; }
    bool                SetFromString(const char *str);

    const char *        ToString(Format format = Digits) const;

    static Guid         FromString(const char *str);

    static int          Hash(const Guid &guid) { return guid[0] + guid[1] + guid[2] + guid[3]; }

    static Guid         CreateGuid();

    static Guid         zero;

private:
    uint32_t            u1, u2, u3, u4;
};

BE_INLINE bool operator==(const Guid &lhs, const Guid &rhs) {
    return ((lhs.u1 ^ rhs.u1) || (lhs.u2 ^ rhs.u2) || (lhs.u3 ^ rhs.u3) || (lhs.u4 ^ rhs.u4)) ? false : true;
}

BE_INLINE bool operator!=(const Guid &lhs, const Guid &rhs) {
    return ((lhs.u1 ^ rhs.u1) || (lhs.u2 ^ rhs.u2) || (lhs.u3 ^ rhs.u3) || (lhs.u4 ^ rhs.u4)) ? true : false;
}

BE_INLINE bool operator<(const Guid &lhs, const Guid &rhs) {
    return  ((lhs.u1 < rhs.u1) ? true : ((lhs.u1 > rhs.u1) ? false :
            ((lhs.u2 < rhs.u2) ? true : ((lhs.u2 > rhs.u2) ? false :
            ((lhs.u3 < rhs.u3) ? true : ((lhs.u3 > rhs.u3) ? false :
            ((lhs.u4 < rhs.u4) ? true : ((lhs.u4 > rhs.u4) ? false : false)))))))); 
}

BE_INLINE bool operator>(const Guid &lhs, const Guid &rhs) {
    return  ((lhs.u1 > rhs.u1) ? true : ((lhs.u1 < rhs.u1) ? false :
            ((lhs.u2 > rhs.u2) ? true : ((lhs.u2 < rhs.u2) ? false :
            ((lhs.u3 > rhs.u3) ? true : ((lhs.u3 < rhs.u3) ? false :
            ((lhs.u4 > rhs.u4) ? true : ((lhs.u4 < rhs.u4) ? false : false))))))));
}

BE_INLINE uint32_t Guid::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return ((uint32_t *)this)[index];
}

BE_INLINE uint32_t &Guid::operator[](int index) {
    assert(index >= 0 && index < 4);
    return ((uint32_t *)this)[index];
}

BE_NAMESPACE_END