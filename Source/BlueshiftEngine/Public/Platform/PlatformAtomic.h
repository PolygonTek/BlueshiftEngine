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

class BE_API PlatformAtomic {
protected:
    PlatformAtomic(const PlatformAtomic &) = delete;
    PlatformAtomic &operator=(const PlatformAtomic &) = delete;

public:
    BE_FORCE_INLINE PlatformAtomic() : data(0) {}
    BE_FORCE_INLINE PlatformAtomic(const atomic_t data) : data(data) {}

    BE_FORCE_INLINE PlatformAtomic &operator=(const atomic_t input) { data = input; return *this; }
    BE_FORCE_INLINE                 operator atomic_t() const { return data; }

    BE_FORCE_INLINE int             GetValue() const { return (int)data; }
    BE_FORCE_INLINE void            SetValue(int value) { data = (atomic_t)value; }

    BE_FORCE_INLINE atomic_t        Add(const atomic_t input) { return atomic_add(&data, input) + input; }
    BE_FORCE_INLINE atomic_t        Sub(const atomic_t input) { return atomic_add(&data, -input) - input; }

    BE_FORCE_INLINE friend atomic_t operator+=(PlatformAtomic &value, const atomic_t input) { return atomic_add(&value.data, input) + input; }
    BE_FORCE_INLINE friend atomic_t operator++(PlatformAtomic &value) { return atomic_add(&value.data, 1) + 1; }
    BE_FORCE_INLINE friend atomic_t operator--(PlatformAtomic &value) { return atomic_add(&value.data, -1) - 1; }
    BE_FORCE_INLINE friend atomic_t operator++(PlatformAtomic &value, int) { return atomic_add(&value.data, 1); }
    BE_FORCE_INLINE friend atomic_t operator--(PlatformAtomic &value, int) { return atomic_add(&value.data, -1); }

    BE_FORCE_INLINE friend atomic_t CompareExchange(PlatformAtomic &value, const atomic_t v, const atomic_t c) { return atomic_cmpxchg(&value.data, v, c); }

private:
    volatile atomic_t               data;
};

BE_NAMESPACE_END


