#pragma once

#include "Shading/Material.h"
#include "Shading/BSDF.h"
#include "Shading/Lambertian.h"
#include "Shading/Microfacet.h"
#include "Shading/DifferentialGeometry.h"

#include "Geometry/ObjLoader.h"
#include "Shading/Texture.h"

class ObjMaterial : public Material {
public:
    ObjMaterial(const ObjMtl &mtl, const char *basedir) {
        BE1::Str path;

        this->kd = mtl.Kd;
        this->diffuseTexture = nullptr;
        if (!mtl.map_Kd.IsEmpty()) {
            path = basedir;

            path.AppendPath(mtl.map_Kd);
            BE1::Image *image = LoadTextureImage(path);
            this->diffuseTexture = new Texture2D(image);
        }

        this->ks = mtl.Ks;
        this->specularTexture = nullptr;
        if (!mtl.map_Ks.IsEmpty()) {
            path = basedir;

            path.AppendPath(mtl.map_Ks);
            BE1::Image *image = LoadTextureImage(path);
            this->specularTexture = new Texture2D(image);
        }

        this->specularExponent = mtl.Ns;
    }

    ~ObjMaterial() override {
        delete diffuseTexture;
        delete specularTexture;
    }

    void                            GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const override;

private:
    BE1::Vec3                       kd;
    Texture2D *                     diffuseTexture;
    BE1::Vec3                       ks;
    Texture2D *                     specularTexture;
    float                           specularExponent;
};

BE_INLINE void ObjMaterial::GetBSDF(const DifferentialGeometry &dg, BSDF *bsdf) const {
    if (!kd.IsZero()) {
        BE1::Vec3 _kd = kd;
        if (diffuseTexture) {
            _kd *= diffuseTexture->Sample(dg.st).ToVec3();
        }
        bsdf->AddBxDF(bsdf->Create<Lambertian>(_kd));
    }

    if (!ks.IsZero()) {
        BE1::Vec3 _ks = ks;
        if (specularTexture) {
            _ks *= specularTexture->Sample(dg.st).ToVec3();
        }
        Fresnel *fresnel = bsdf->Create<FresnelDielectric>(1.0f, 1.5f);
        MicrofacetDistribution *md = bsdf->Create<BlinnDistribution>(specularExponent);
        bsdf->AddBxDF(bsdf->Create<Microfacet>(_ks, fresnel, md));
    }
}
