#pragma once

#include "Shading/DifferentialGeometry.h"

//-------------------------------------------------------------------
// Base BxDF
//-------------------------------------------------------------------

class BxDF {
public:
    enum Type {
        DiffuseReflection = 0x01,
        GlossyReflection = 0x02,
        SpecularReflection = 0x04,
        DiffuseTransmission = 0x08,
        GlossyTransmission = 0x10,
        SpecularTransmission = 0x20,

        Diffuse = DiffuseReflection | DiffuseTransmission,
        Glossy = GlossyReflection | GlossyTransmission,
        Specular = SpecularReflection | SpecularTransmission,

        Reflection = DiffuseReflection | GlossyReflection | SpecularReflection,
        Transmission = DiffuseTransmission | GlossyTransmission | SpecularTransmission,
        All = Reflection | Transmission
    };

    BxDF(int type) { this->type = type; }

    int                             GetType() const { return type; }
    bool                            CheckType(int type) const { return (this->type & type) ? true : false; }

                                    // Return BxDF * cos_in
    virtual BE1::Vec3               Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const { return BE1::Vec3::zero; }

                                    // Return BxDF * cos_in with wi vector and pdf
    virtual BE1::Vec3               SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const { return BE1::Vec3::zero; }

                                    // Probability density function with respect to wi vector with a given vector wo
    virtual float                   PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const = 0;

protected:
    int                             type;
};
