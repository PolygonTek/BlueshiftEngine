#pragma once

#include "Shading/BxDF.h"

//-------------------------------------------------------------------
// Phong
//-------------------------------------------------------------------

class Phong : public BxDF {
public:
    Phong(const BE1::Vec3 &r, const float kd, const float ks, const float exp);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
    float                           kd;
    float                           ks;
    float                           exp;
};

BE_INLINE Phong::Phong(const BE1::Vec3 &r, const float kd, const float ks, const float exp)
    : BxDF(BxDF::DiffuseReflection | BxDF::GlossyReflection) {
    this->r = r;
    this->kd = kd;
    this->ks = ks;
    this->exp = exp;
}
