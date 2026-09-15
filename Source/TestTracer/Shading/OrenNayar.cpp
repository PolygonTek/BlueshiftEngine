#include "Precompiled.h"
#include "Shading/OrenNayar.h"
#include "Shading/BxDF.h"
#include "Render/Sampler.h"

BE1::Vec3 OrenNayar::Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float NdotL = BE1::Max(dg.sn.Dot(wi), 0.0f);
    float NdotV = BE1::Max(dg.sn.Dot(wo), 0.0f);
    float theta_i = acos(NdotL);
    float theta_o = acos(NdotV);
    float alpha = BE1::Max(theta_i, theta_o);
    float beta = BE1::Min(theta_i, theta_o);

    BE1::Vec3 tangentL = wi - dg.sn * NdotL;
    tangentL.Normalize();
    BE1::Vec3 tangentV = wo - dg.sn * NdotV;
    tangentV.Normalize();

    float gamma = tangentL.Dot(tangentV);
    return r * (A + B * BE1::Max(0.0f, gamma) * sinf(alpha) * tanf(beta)) * BE1::Math::InvPi * NdotL;
}

BE1::Vec3 OrenNayar::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &, BE1::Vec3 *wi, float *pdf) const {
    BE1::Vec3 sampleDir = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
    *wi = RotateWithUpVector(sampleDir, dg.sn);
    *pdf = PDF(dg, *wi, wo);
    return Eval(dg, *wi, wo);
}

float OrenNayar::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float cos_in = dg.sn.Dot(wi);
    return BE1::SamplingPDF::CosineHemisphere(cos_in);
}
