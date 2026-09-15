#pragma once

#include "BlueshiftEngine.h"

class Fresnel {
public:
    virtual BE1::Vec3               Eval(float cos_in) const = 0;
};

class FresnelNoOp : public Fresnel {
public:
    BE1::Vec3                       Eval(float cos_in) const override;
};

class FresnelConductor : public Fresnel {
public:
    FresnelConductor(const BE1::Vec3 &eta, const BE1::Vec3 &k) {
        this->eta = eta;
        this->k = k;
    }

    BE1::Vec3                       Eval(float cos_in) const override;

private:
    BE1::Vec3                       eta;
    BE1::Vec3                       k;
};

class FresnelDielectric : public Fresnel {
public:
    FresnelDielectric(float etai, float etat) {
        this->etai = etai;
        this->etat = etat;
    }

    BE1::Vec3                       Eval(float cos_in) const override;

private:
    float                           etai;
    float                           etat;
};
