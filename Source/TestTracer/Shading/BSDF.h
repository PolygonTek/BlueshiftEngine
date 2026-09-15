#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include "BlueshiftEngine.h"

class BxDF;
class DifferentialGeometry;

class BSDF {
private:
    static constexpr int            MaxComponents = 8;
    static constexpr size_t         MaxBytes = 256 * sizeof(float);

public:
    BSDF() = default;
    // BxDFs and their dependencies can point into this object's storage.
    BSDF(const BSDF &) = delete;
    BSDF &operator=(const BSDF &) = delete;
    BSDF(BSDF &&) = delete;
    BSDF &operator=(BSDF &&) = delete;

                                    // Objects live in the BSDF buffer; no destructors are run.
    template <typename T, typename... Args>
    T *                             Create(Args &&...args);

    void                            AddBxDF(const BxDF *bxdf);
    int                             NumBxDFs() const { return numBxDFs; }
    const BxDF *                    operator[](size_t i) const;

    BE1::Vec3                       Eval(int type, const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const;
    BE1::Vec3                       SampleEval(int type, const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf, int *outType) const;

private:
    int                             numBxDFs = 0;
    const BxDF *                    bxdfs[MaxComponents];

    alignas(std::max_align_t) char  data[MaxBytes];
    size_t                          offset = 0;
};

template <typename T, typename... Args>
BE_INLINE T *BSDF::Create(Args &&...args) {
    static_assert(alignof(T) <= alignof(std::max_align_t), "BSDF storage does not support over-aligned types");
    static_assert(sizeof(T) <= MaxBytes, "Object exceeds BSDF storage capacity");
    static_assert(std::is_trivially_destructible<T>::value, "BSDF objects must not require destruction");

    const size_t alignedOffset = (offset + alignof(T) - 1) & ~(alignof(T) - 1);
    assert(alignedOffset <= MaxBytes && sizeof(T) <= MaxBytes - alignedOffset);

    void *p = &data[alignedOffset];
    offset = alignedOffset + sizeof(T);
    return ::new (p) T(std::forward<Args>(args)...);
}

BE_INLINE const BxDF *BSDF::operator[](size_t i) const {
    assert(i < static_cast<size_t>(numBxDFs));
    return bxdfs[i];
}

BE_INLINE void BSDF::AddBxDF(const BxDF *bxdf) {
    assert(numBxDFs < MaxComponents);
    if (numBxDFs < MaxComponents) {
        bxdfs[numBxDFs++] = bxdf;
    }
}
