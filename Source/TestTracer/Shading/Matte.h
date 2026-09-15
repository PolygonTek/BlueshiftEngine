#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Lambertian.h"
#include "Shading/OrenNayar.h"

class MatteMaterial : public Material {
public:
    MatteMaterial(const BE1::Vec3 &kd, float sigma) {
        this->kd = kd;
        this->sigma = sigma;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       kd;
    float                           sigma;
};

BE_INLINE void MatteMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    if (sigma == 0.0f) {
        bsdf->AddBxDF(bsdf->Create<Lambertian>(kd));
    } else {
        bsdf->AddBxDF(bsdf->Create<OrenNayar>(kd, sigma));
    }
}
