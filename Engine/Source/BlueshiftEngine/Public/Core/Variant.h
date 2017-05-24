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

BE_NAMESPACE_BEGIN

class BE_API Variant {
public:
    Variant();
    Variant(const char *text);
    template <typename T>   Variant(const T &value);

    Variant &               operator=(const Variant &rhs);
    template <typename T>
    Variant &               operator=(const T &rhs);

    bool                    operator==(const Variant &rhs) const;
    bool                    operator!=(const Variant &rhs) const { return !(*this == rhs); }

    template <typename T>
    const T &               As() const { return *reinterpret_cast<const T *>(&pod); }
    template <typename T>
    T &                     As() { return *reinterpret_cast<T *>(&pod); }
    
    void                    SetEmpty();

private:
    union Pod {
        int32_t             i[4];   // integer, Point, Rect, Guid
        float               f[16];  // float, Vec2, Vec3, Vec4, Angles, Mat3, Mat4
        bool                b;      // bool
    };
    Pod                     pod;
    int                     podSize;
    Str                     str;
};

BE_INLINE Variant::Variant() {
    podSize = 0;
}

BE_INLINE Variant::Variant(const char *text) {
    podSize = 0;
    str = text;
}

template <typename T>
BE_INLINE Variant::Variant(const T &value) {
    podSize = sizeof(T);
    assert(podSize <= sizeof(Pod));
    *reinterpret_cast<T *>(&pod) = value;
}

template <>
BE_INLINE Variant::Variant(const Str &value) {
    podSize = 0;
    str = value;
}

BE_INLINE Variant &Variant::operator=(const Variant &rhs) {
    if (rhs.podSize > 0) {
        memcpy(&pod, &rhs.pod, rhs.podSize);
        podSize = rhs.podSize;
    } else {
        str = rhs.str;
        podSize = 0;
    }
    return *this;
}

template <typename T>
BE_INLINE Variant &Variant::operator=(const T &rhs) {
    podSize = sizeof(T);
    assert(podSize <= sizeof(Pod));
    *reinterpret_cast<T *>(&pod) = rhs;
    return *this;
}

template <>
BE_INLINE Variant &Variant::operator=(const Str &rhs) {
    podSize = 0;
    str = rhs;
    return *this;
}

BE_INLINE bool Variant::operator==(const Variant &rhs) const {
    if (podSize != rhs.podSize) {
        return false;
    }

    if (podSize > 0) {
        int cmp = memcmp(&pod, &rhs.pod, podSize);
        return cmp == 0 ? true : false;
    }
    return str == rhs.str;
}

template <>
BE_INLINE const Str &Variant::As() const {
    return str;
}

template <>
BE_INLINE Str &Variant::As() {
    return str;
}

BE_INLINE void Variant::SetEmpty() {
    memset(&pod, 0, sizeof(pod));
    podSize = 0;
    str.Clear();
}

BE_NAMESPACE_END
