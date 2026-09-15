#pragma once

#include "Shading/Fresnel.h"

#include "Shading/BxDF.h"

//-------------------------------------------------------------------
// Specular Reflection
//-------------------------------------------------------------------

class SpecularReflection : public BxDF {
public:
    SpecularReflection(const BE1::Vec3 &r, const Fresnel *fresnel);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
    const Fresnel *                 fresnel;
};

BE_INLINE SpecularReflection::SpecularReflection(const BE1::Vec3 &r, const Fresnel *fresnel)
    : BxDF(BxDF::SpecularReflection) {
    this->r = r;
    this->fresnel = fresnel;
}

//-------------------------------------------------------------------
// Specular Transmission
//-------------------------------------------------------------------

class SpecularTransmission : public BxDF {
public:
    SpecularTransmission(const BE1::Vec3 &r, float etai, float etat);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
    float                           etai, etat;
    FresnelDielectric               fresnel;
};

BE_INLINE SpecularTransmission::SpecularTransmission(const BE1::Vec3 &r, float etai, float etat)
    : BxDF(BxDF::SpecularTransmission), fresnel(etai, etat) {
    this->r = r;
    this->etai = etai;
    this->etat = etat;
}
