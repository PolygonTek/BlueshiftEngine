#include "Precompiled.h"
#include "Shading/Specular.h"
#include "Shading/BxDF.h"
#include "Render/Sampler.h"

BE1::Vec3 SpecularReflection::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &, BE1::Vec3 *wi, float *pdf) const {
    *wi = wo.Reflect(dg.sn);
    *pdf = 1.0f;
    return r * fresnel->Eval(dg.sn.Dot(wo));
}

float SpecularReflection::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    return 1.0f;
}

BE1::Vec3 SpecularTransmission::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &, BE1::Vec3 *wi, float *pdf) const {
    float ei = etai;
    float et = etat;
    if (wo.Dot(dg.sn) < 0.0f) {
        BE1::Swap(ei, et);
    }
    *wi = wo.Refract(dg.sn, ei / et);
    *pdf = 1.0f;

    BE1::Vec3 f = fresnel.Eval(dg.sn.Dot(wo));
    return r * (1.0f - f);
}

float SpecularTransmission::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    return 1.0f;
}
