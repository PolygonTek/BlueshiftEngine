#pragma once

#include "Shading/BxDF.h"

//-------------------------------------------------------------------
// Lambert
//-------------------------------------------------------------------

class Lambertian : public BxDF {
public:
    Lambertian(const BE1::Vec3 &r);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
};

BE_INLINE Lambertian::Lambertian(const BE1::Vec3 &r)
    : BxDF(BxDF::DiffuseReflection) {
    this->r = r;
}
