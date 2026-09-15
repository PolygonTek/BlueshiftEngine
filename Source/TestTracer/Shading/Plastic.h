#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Lambertian.h"
#include "Shading/Microfacet.h"

class PlasticMaterial : public Material {
public:
    PlasticMaterial(const BE1::Vec3 &kd, const BE1::Vec3 &ks, float roughness) {
        this->kd = kd;
        this->ks = ks;
        this->roughness = roughness;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       kd;
    BE1::Vec3                       ks;
    float                           roughness;
};

BE_INLINE void PlasticMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    bsdf->AddBxDF(bsdf->Create<Lambertian>(kd));
    Fresnel *fresnel = bsdf->Create<FresnelDielectric>(1.0f, 1.5f);
    MicrofacetDistribution *md = bsdf->Create<BlinnDistribution>(1.0f / roughness);
    bsdf->AddBxDF(bsdf->Create<Microfacet>(ks, fresnel, md));
}
