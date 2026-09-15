#include "Precompiled.h"
#include "Shading/Fresnel.h"

struct IorEntry {
    const char *name;
    float value;
};

static IorEntry ior[] = {
    {"vacuum", 1.0f},
    {"helium", 1.000036f},
    {"hydrogen", 1.000132f},
    {"air", 1.000277f},
    {"carbon dioxide", 1.00045f},
    //////////////////////////////////////
    {"water", 1.3330f},
    {"acetone", 1.36f},
    {"ethanol", 1.361f},
    {"carbon tetrachloride", 1.461f},
    {"glycerol", 1.4729f},
    {"benzene", 1.501f},
    {"silicone oil", 1.52045f},
    {"bromine", 1.661f},
    //////////////////////////////////////
    {"water ice", 1.31f},
    {"fused quartz", 1.458f},
    {"pyrex", 1.470f},
    {"acrylic glass", 1.49f},
    {"polypropylene", 1.49f},
    {"bk7", 1.5046f},
    {"sodium chloride", 1.544f},
    {"amber", 1.55f},
    {"pet", 1.5750f},
    {"diamond", 2.419f},

    {nullptr, 0.0f}};

// Fresnel Reflectance for Dielectrics
static BE_FORCE_INLINE BE1::Vec3 FrDiel(float cos_in, float cos_tn, const BE1::Vec3 &etai, const BE1::Vec3 &etat) {
    BE1::Vec3 etat_cos_in = etat * cos_in;
    BE1::Vec3 etai_cos_tn = etai * cos_tn;
    BE1::Vec3 etai_cos_in = etai * cos_in;
    BE1::Vec3 etat_cos_tn = etat * cos_tn;

    BE1::Vec3 rparl = (etat_cos_in - etai_cos_tn) / (etat_cos_in + etai_cos_tn);
    BE1::Vec3 rperp = (etai_cos_in - etat_cos_tn) / (etai_cos_in + etat_cos_tn);

    return (rparl * rparl + rperp * rperp) * 0.5f;
}

// Fresnel Reflectance for Conductors
static BE_FORCE_INLINE BE1::Vec3 FrCond(float cos_in, const BE1::Vec3 &eta, const BE1::Vec3 &k) {
    BE1::Vec3 eta2_k2 = eta * eta + k * k;
    float cos_in2 = cos_in * cos_in;
    BE1::Vec3 eta2_k2_cos_in2 = eta2_k2 * cos_in2;
    BE1::Vec3 eta_cos_in_2 = eta * cos_in * 2.0f;

    BE1::Vec3 rparl2 = (eta2_k2_cos_in2 - eta_cos_in_2 + 1.0f) / (eta2_k2_cos_in2 + eta_cos_in_2 + 1.0f);
    BE1::Vec3 rperp2 = (eta2_k2 - eta_cos_in_2 + cos_in2) / (eta2_k2 + eta_cos_in_2 + cos_in2);

    return (rparl2 + rperp2) * 0.5f;
}

BE1::Vec3 FresnelNoOp::Eval(float cos_in) const {
    return BE1::Vec3(1.0f, 1.0f, 1.0f);
}

BE1::Vec3 FresnelConductor::Eval(float cos_in) const {
    return FrCond(cos_in, eta, k);
}

BE1::Vec3 FresnelDielectric::Eval(float cos_in) const {
    float ei = etai;
    float et = etat;
    if (cos_in < 0.0f) {
        BE1::Swap(ei, et);
    }

    // Compute cos_tn using Snell's law
    float ei_et = ei / et;
    float cos_tn2 = 1.0f - ei_et * ei_et * (1.0f - cos_in * cos_in);
    if (cos_tn2 <= 0.0f) {
        return BE1::Vec3(1.0f, 1.0f, 1.0f);
    }

    float cos_tn = BE1::Math::Sqrt(cos_tn2);
    return FrDiel(BE1::Math::Abs(cos_in), cos_tn, BE1::Vec3(ei), BE1::Vec3(et));
}
