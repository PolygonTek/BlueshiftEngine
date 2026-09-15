#include "Precompiled.h"
#include "Shading/Microfacet.h"
#include "Shading/BxDF.h"
#include "Render/Sampler.h"

BlinnDistribution::BlinnDistribution(float exp) {
    assert(exp >= 0.0f);
    if (exp < 10000.0f) {
        this->exp = exp;
    } else {
        this->exp = 10000.0f;
    }
}

BE1::Vec3 BlinnDistribution::SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const {
    float costheta = powf(u.x, 1.0f / (exp + 1));
    float sintheta = BE1::Math::Sqrt(1.0f - costheta * costheta);
    float phi = BE1::Math::TwoPi * u.y;
    float sinphi, cosphi;
    BE1::Math::SinCos(phi, sinphi, cosphi);

    BE1::Vec3 wh = BE1::Vec3(sintheta * cosphi, sintheta * sinphi, costheta);
    wh = RotateWithUpVector(wh, normal);
    wh.Normalize();
    if (normal.Dot(wo) * normal.Dot(wh) < 0.0f) {
        wh = -wh;
    }

    return wo.Reflect(wh);
}

float BlinnDistribution::Eval(const BE1::Vec3 &wh, float NdotH) const {
    if (NdotH <= 0.0f) {
        return 0.0f;
    }
    float ret = (exp + 2.0f) * BE1::Math::InvTwoPi * powf(NdotH, exp);
    assert(!std::isnan(ret));
    return ret;
}

float BlinnDistribution::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    BE1::Vec3 wh = wi + wo;
    wh.Normalize();
    float VdotH = wo.Dot(wh);
    if (VdotH <= 0.0f) {
        return 0.0f;
    }
    float NdotH = BE1::Math::Abs(dg.sn.Dot(wh));
    float ret = ((exp + 1.0f) * powf(NdotH, exp) * BE1::Math::InvTwoPi) / (4.0f * VdotH);
    assert(!std::isnan(ret));
    return ret;
}

BeckmannDistribution::BeckmannDistribution(float exp) {
    assert(exp >= 0.0f);
    if (exp < 10000.0f) {
        this->exp = exp;
    } else {
        this->exp = 10000.0f;
    }
}

BE1::Vec3 BeckmannDistribution::SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const {
    BE1::Vec3 wi = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
    return RotateWithUpVector(wi, normal);
}

float BeckmannDistribution::Eval(const BE1::Vec3 &wh, float NdotH) const {
    float e2 = exp * exp;
    float NdotH2 = NdotH * NdotH;
    return expf((NdotH2 - 1.0f) / (e2 * NdotH2)) / (BE1::Math::Pi * e2 * NdotH2 * NdotH2);
}

float BeckmannDistribution::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float cos_in = dg.sn.Dot(wi);
    return BE1::SamplingPDF::CosineHemisphere(cos_in);
}

AnisotropicDistribution::AnisotropicDistribution(float exp_x, float exp_y) {
    assert(exp_x >= 0.0f && exp_y >= 0.0f);
    if (exp_x < 10000.0f) {
        this->exp_x = exp_x;
    } else {
        this->exp_x = 10000.0f;
    }
    if (exp_y < 10000.0f) {
        this->exp_y = exp_y;
    } else {
        this->exp_y = 10000.0f;
    }
}

BE1::Vec3 AnisotropicDistribution::SampleDirection(const BE1::Vec2 &u, const BE1::Vec3 &normal, const BE1::Vec3 &wo) const {
    BE1::Vec3 wi = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
    return RotateWithUpVector(wi, normal);
}

float AnisotropicDistribution::Eval(const BE1::Vec3 &wh, float NdotH) const {
    float d = 1.0f - NdotH * NdotH;
    if (d == 0.0f)
        return 0.0f;
    float exp = (exp_x * wh.x * wh.x + exp_y * wh.y * wh.y) / d;
    return BE1::Math::Sqrt((exp_x + 2.0f) * (exp_y + 2.0f)) * BE1::Math::InvTwoPi * powf(NdotH, exp);
}

float AnisotropicDistribution::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float cos_in = dg.sn.Dot(wi);
    return BE1::SamplingPDF::CosineHemisphere(cos_in);
}

BE1::Vec3 Microfacet::Eval(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    float NdotL = dg.sn.Dot(wi);
    float NdotV = dg.sn.Dot(wo);

    if (NdotL <= 0.0f || NdotV <= 0.0f) {
        return BE1::Vec3::zero;
    }

    BE1::Vec3 wh = wi + wo;
    wh.Normalize();
    float NdotH = dg.sn.Dot(wh);
    float VdotH = wo.Dot(wh);

    BE1::Vec3 F = fresnel->Eval(NdotL);

#if 0
    float reflectanceAtNormal = 0.5f;

    // Compute fresnel term (Schlick’s approximation)
    float oneMinusVdotH = 1.0f - VdotH;
    float oneMinusVdotH2 = oneMinusVdotH * oneMinusVdotH;
    float F = reflectanceAtNormal + oneMinusVdotH2 * oneMinusVdotH2 * oneMinusVdotH * (1.0f - reflectanceAtNormal);

    // Diffuse reflectance considered fresnel incidence effect (for Schlick approximatino)
    /*float diffuse = 28.0f / (23.0f * Math::PI) * (1.0f - ks) *
        (1.0f - powf(1.0f - NdotL * 0.5f, 5.0f)) *
        (1.0f - powf(1.0f - NdotV * 0.5f, 5.0f));*/
#endif

    // Compute Geometric term
    float numerator = 2.0f * NdotH / VdotH;
    float G = BE1::Min3(1.0f, numerator * NdotV, numerator * NdotL);

    // Specular reflectance
    return r * distribution->Eval(wh, NdotH) * G * F / (4.0f * NdotV);
}

BE1::Vec3 Microfacet::SampleEval(const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &, BE1::Vec3 *wi, float *pdf) const {
    *wi = distribution->SampleDirection(u, dg.sn, wo);
    *pdf = 0.0f;

    if (dg.sn.Dot(*wi) <= 0.0f || dg.sn.Dot(wo) <= 0.0f) {
        return BE1::Vec3::zero;
    }

    *pdf = distribution->PDF(dg, *wi, wo);
    return Eval(dg, *wi, wo);
}

float Microfacet::PDF(const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    if (dg.sn.Dot(wi) <= 0.0f || dg.sn.Dot(wo) <= 0.0f) {
        return 0.0f;
    }

    return distribution->PDF(dg, wi, wo); // / dg.sn.Dot(wi);
}
