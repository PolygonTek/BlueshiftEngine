#include "Precompiled.h"
#include "Shading/Lambertian.h"
#include "Shading/BxDF.h"
#include "Render/Sampler.h"

BE1::Vec3 Lambertian::Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    return r * BE1::Math::InvPi * BE1::Max(dg.sn.Dot(wi), 0.0f);
}

BE1::Vec3 Lambertian::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &, BE1::Vec3 *wi, float *pdf) const {
    BE1::Vec3 sampleDir = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
    *wi = RotateWithUpVector(sampleDir, dg.sn);
    *pdf = PDF(dg, *wi, wo);
    return Eval(dg, *wi, wo);
}

float Lambertian::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float cos_in = dg.sn.Dot(wi);
    return BE1::SamplingPDF::CosineHemisphere(cos_in);
}
