#include "Precompiled.h"
#include "Shading/Phong.h"
#include "Shading/BxDF.h"
#include "Render/Sampler.h"

BE1::Vec3 Phong::Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float NdotL = dg.sn.Dot(wi);
    float diffuse = BE1::Math::InvPi * BE1::Max(NdotL, 0.0f);
    BE1::Vec3 s = wo.Reflect(dg.sn);
    float SdotL = BE1::Max(s.Dot(wi), 0.0f);
    float specular = (exp + 2) * powf(SdotL, exp) * BE1::Math::InvTwoPi;

    return r * (kd * diffuse + ks * specular);
}

BE1::Vec3 Phong::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf) const {
    BE1::Vec3 sampleDir;
    float sel = rng.GetNextFloat();
    if (sel < kd) {
        sampleDir = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
        *wi = RotateWithUpVector(sampleDir, dg.sn);
    } else {
        sampleDir = BE1::Vec3::FromPowerCosineSampleHemisphere(u.x, u.y, exp);
        BE1::Vec3 s = wo.Reflect(dg.sn);
        *wi = RotateWithUpVector(sampleDir, s);
    }

    *pdf = PDF(dg, *wi, wo);
    return Eval(dg, *wi, wo);
}

float Phong::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float cos_in = dg.sn.Dot(wi);
    float pd = BE1::SamplingPDF::CosineHemisphere(cos_in);
    BE1::Vec3 s = wo.Reflect(dg.sn);
    float ps = BE1::SamplingPDF::PowerCosineHemisphere(s.Dot(wi), exp);

    return kd * pd + ks * ps;
}
