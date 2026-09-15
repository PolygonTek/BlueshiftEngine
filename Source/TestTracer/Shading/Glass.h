#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Specular.h"

class GlassMaterial : public Material {
public:
    GlassMaterial(const BE1::Vec3 &r, const BE1::Vec3 &t, const float ior) {
        this->r = r;
        this->t = t;
        this->ior = ior;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       r;
    BE1::Vec3                       t;
    float                           ior;
};

BE_INLINE void GlassMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    if (!r.IsZero()) {
        Fresnel *fresnel = bsdf->Create<FresnelDielectric>(1.0f, ior);
        bsdf->AddBxDF(bsdf->Create<SpecularReflection>(r, fresnel));
    }
    if (!t.IsZero()) {
        bsdf->AddBxDF(bsdf->Create<SpecularTransmission>(t, 1.0f, ior));
    }
}
