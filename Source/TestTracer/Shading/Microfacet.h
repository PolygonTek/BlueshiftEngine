#pragma once

#include "Shading/Fresnel.h"
#include "Shading/BxDF.h"
#include "Shading/DifferentialGeometry.h"

class MicrofacetDistribution {
public:
    MicrofacetDistribution() = default;

    virtual BE1::Vec3               SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const = 0;
    virtual float                   Eval(const BE1::Vec3 &wh, float NdotH) const = 0;
    virtual float                   PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const = 0;
};

class BlinnDistribution : public MicrofacetDistribution {
public:
    BlinnDistribution(float exp);

    BE1::Vec3                       SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const override;
    float                           Eval(const BE1::Vec3 &wh, float NdotH) const override;
    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;

private:
    float                           exp;
};

class BeckmannDistribution : public MicrofacetDistribution {
public:
    BeckmannDistribution(float exp);

    BE1::Vec3                       SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const override;
    float                           Eval(const BE1::Vec3 &wh, float NdotH) const override;
    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;

private:
    float                           exp;
};

class AnisotropicDistribution : public MicrofacetDistribution {
public:
    AnisotropicDistribution(float exp_x, float exp_y);

    BE1::Vec3                       SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const override;
    float                           Eval(const BE1::Vec3 &wh, float NdotH) const override;
    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;

private:
    float                           exp_x;
    float                           exp_y;
};

//-------------------------------------------------------------------
// Torrance-Sparrow Microfacet Model
//-------------------------------------------------------------------

class Microfacet : public BxDF {
public:
    Microfacet(const BE1::Vec3 &r, const Fresnel *fresnel, const MicrofacetDistribution *distribution);

    float                           PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const override;
    BE1::Vec3                       SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const override;

private:
    BE1::Vec3                       r;
    const Fresnel *                 fresnel;
    const MicrofacetDistribution *  distribution;
};

BE_INLINE Microfacet::Microfacet(const BE1::Vec3 &r, const Fresnel *fresnel, const MicrofacetDistribution *distribution)
    : BxDF(BxDF::GlossyReflection) {
    this->r = r;
    this->fresnel = fresnel;
    this->distribution = distribution;
}
