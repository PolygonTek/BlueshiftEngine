#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Microfacet.h"

class MetalMaterial : public Material {
public:
    MetalMaterial(const BE1::Vec3 &eta, const BE1::Vec3 &k, float roughness) {
        this->eta = eta;
        this->k = k;
        this->roughness = roughness;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       eta;
    BE1::Vec3                       k;
    float                           roughness;
};

BE_INLINE void MetalMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    Fresnel *fresnel = bsdf->Create<FresnelConductor>(eta, k);
    MicrofacetDistribution *md = bsdf->Create<BlinnDistribution>(1.0f / roughness);
    bsdf->AddBxDF(bsdf->Create<Microfacet>(BE1::Vec3(1.0f, 1.0f, 1.0f), fresnel, md));
}
