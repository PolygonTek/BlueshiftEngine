#pragma once

#include "Platform/Intrinsics.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformAtomic {
protected:
    PlatformAtomic(const PlatformAtomic &); // don't implement
    PlatformAtomic &operator=(const PlatformAtomic &); // don't implement

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


