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

#include "Platform/Intrinsics.h"

BE_NAMESPACE_BEGIN

template <typename T>
class BE_API PlatformAtomic {
public:
    PlatformAtomic(const PlatformAtomic &) = delete;
    PlatformAtomic &operator=(const PlatformAtomic &) = delete;

    BE_FORCE_INLINE PlatformAtomic() : value(0) {}
    BE_FORCE_INLINE PlatformAtomic(const T v) : value(v) {}

    BE_FORCE_INLINE                 operator T() const { return value; }

    BE_FORCE_INLINE PlatformAtomic &operator=(const T v) { SetValue(v); return *this; }

    BE_FORCE_INLINE T               GetValue() const { return value; }
    BE_FORCE_INLINE T               SetValue(int v) { return atomic_xchg(&value, v); }

                                    /// Adds a value from this counter and returns old value.
    BE_FORCE_INLINE T               Add(const T v) { return atomic_add(&value, +v); }

                                    /// Subtracts a value from this counter and returns old value.
    BE_FORCE_INLINE T               Sub(const T v) { return atomic_add(&value, -v); }

    BE_FORCE_INLINE PlatformAtomic &operator+=(const T v) { atomic_add(&value, +v); return *this; }
    BE_FORCE_INLINE PlatformAtomic &operator-=(const T v) { atomic_add(&value, -v); return *this; }

private:
    volatile T                      value;
};

BE_NAMESPACE_END
