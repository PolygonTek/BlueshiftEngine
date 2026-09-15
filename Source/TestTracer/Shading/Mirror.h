#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Specular.h"

class MirrorMaterial : public Material {
public:
    MirrorMaterial(const BE1::Vec3 &r) {
        this->r = r;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       r;
};

BE_INLINE void MirrorMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    if (!r.IsZero()) {
        Fresnel *fresnel = bsdf->Create<FresnelNoOp>();
        bsdf->AddBxDF(bsdf->Create<SpecularReflection>(r, fresnel));
    }
}
