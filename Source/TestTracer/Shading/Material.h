#pragma once

#include "BlueshiftEngine.h"

class DifferentialGeometry;
class BSDF;

class Material {
public:
    Material() { le.SetFromScalar(0); }
    Material(const Material &) = delete;
    Material &                      operator=(const Material &) = delete;
    virtual ~Material() = default;

    virtual void                    GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const = 0;

    BE1::Vec3                       le;
};

class EmissiveMaterial : public Material {
public:
    EmissiveMaterial(const BE1::Vec3 &le) { this->le = le; }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override {}
};
