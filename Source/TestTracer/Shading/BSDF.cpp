#include "Precompiled.h"
#include "Shading/BSDF.h"
#include "Shading/BxDF.h"

BE1::Vec3 BSDF::Eval(int type, const DifferentialGeometry &dg, const BE1::Vec3 &wi, const BE1::Vec3 &wo) const {
    BE1::Vec3 c = BE1::Vec3::zero;
    for (int i = 0; i < numBxDFs; i++) {
        if (bxdfs[i]->CheckType(type)) {
            c += bxdfs[i]->Eval(dg, wi, wo);
        }
    }
    return c;
}

BE1::Vec3 BSDF::SampleEval(int type, const DifferentialGeometry &dg, const BE1::Vec3 &wo, const BE1::Vec2 &u, BE1::RNG &rng, BE1::Vec3 *wi, float *pdf, int *outType) const {
    const float s = rng.GetNextFloat();
    float f[MaxComponents];
    BE1::Vec3 colors[MaxComponents];
    BE1::Vec3 samples[MaxComponents];
    float pdfs[MaxComponents];
    int types[MaxComponents];
    int num = 0;
    float sum = 0.0f;

    for (int i = 0; i < numBxDFs; i++) {
        if (!(bxdfs[i]->CheckType(type))) {
            continue;
        }

        BE1::Vec3 wi;
        float pdf;
        BE1::Vec3 c = bxdfs[i]->SampleEval(dg, wo, u, rng, &wi, &pdf);
        if (c.IsZero() || pdf <= 0.0f) {
            continue;
        }

        sum += f[num] = (c.x + c.y + c.z) / pdf;
        colors[num] = c;
        samples[num] = wi;
        pdfs[num] = pdf;
        types[num] = bxdfs[i]->GetType();
        num++;
    }

    if (num == 0) {
        *wi = BE1::Vec3::zero;
        return BE1::Vec3::zero;
    }

    // Normalize distribution
    for (int i = 0; i < num; i++) {
        f[i] /= sum;
    }

    // Compute accumulated distribution
    float d[MaxComponents];
    d[0] = f[0];
    for (int i = 1; i < num - 1; i++) {
        d[i] = d[i - 1] + f[i];
    }
    d[num - 1] = 1.0f;

    // Sample distribution
    int idx = 0;
    while (idx < num - 1 && s > d[idx]) {
        idx++;
    }

    *wi = samples[idx];
    *pdf = pdfs[idx] * f[idx];
    *outType = types[idx];
    return colors[idx];
}
