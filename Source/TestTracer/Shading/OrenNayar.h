#pragma once

#include "Shading/BxDF.h"

//-------------------------------------------------------------------
// Oren-Nayar
//-------------------------------------------------------------------

class OrenNayar : public BxDF {
public:
    OrenNayar(const BE1::Vec3 &r, const float sigma);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
    float                           A, B;
};

BE_INLINE OrenNayar::OrenNayar(const BE1::Vec3 &r, const float sigma)
    : BxDF(BxDF::DiffuseReflection) {
    this->r = r;
    float sigma2 = sigma * sigma;
    A = 1.0f - 0.5f * sigma2 / (sigma2 + 0.33f);
    B = 0.45f * sigma2 / (sigma2 + 0.09f);
}
