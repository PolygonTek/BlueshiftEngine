#include "Precompiled.h"
#include "Render/PathTracer.h"
#include "Scene/Scene.h"
#include "Shading/BxDF.h"
#include "Shading/BSDF.h"
#include "Shading/Material.h"
#include "Geometry/BVH.h"
#include "Geometry/Mesh.h"
#include "Geometry/Model.h"
#include "Render/Sampler.h"
#include "Shading/Texture.h"

PathTracer::PathTracer(const SceneManager &scene, const Options &options, float unitMeter, Sampler &splitSampler, Sampler &shadowSampler, uint64_t seed)
    : scene(scene), options(options), splitSampleCount(options.splitSampleCount),
      shadowSampleCount(options.shadowSampleCount), splitSampler(splitSampler), shadowSampler(shadowSampler),
      rng(seed), unitMeter(unitMeter), epsilon(0.00001f * unitMeter) {
    assert(seed != 0);
}

bool PathTracer::RayNearestIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, DifferentialGeometry *dg) const {
    const Material *material;
    BE1::Vec3 n;
    BE1::Vec3 gn, sn;
    BE1::Vec2 st;
    HitTri hit;
    float minDist;
    float exitDist;
    float dist;

    numRays++;

    minDist = FLT_MAX;

    for (unsigned int i = 0; i < scene.GetShapes().Count(); i++) {
        const Shape *shape = &scene.GetShapes()[i];

        BE1::Vec3 localStart = shape->axis.TransposedMulVec(start - shape->position);
        BE1::Vec3 localDir = shape->axis.TransposedMulVec(dir);

        if (shape->type == Shape::PLANE) {
            if (!shape->plane.IntersectRay(BE1::Ray(localStart, localDir), false, &dist)) {
                continue;
            }

            n = shape->plane.normal;
        } else if (shape->type == Shape::SPHERE) {
            if (!shape->sphere.IntersectRay(BE1::Ray(localStart, localDir), &dist, &exitDist)) {
                continue;
            }
            // A ray inside a transmissive sphere must use the exit surface.
            if (dist <= 0.0f)
                dist = exitDist;
            n = localStart + localDir * dist - shape->sphere.Center();
            n.Normalize();
        } else if (shape->type == Shape::MODEL) {
            if (!shape->model->RayIntersection(localStart, localDir, &hit, minDist)) {
                continue;
            }
        }

        if (shape->type == Shape::MODEL) {
            dist = hit.t;

            const SubMesh *mesh = shape->model->GetMesh(hit.meshIndex);
            material = mesh->material;
            const Vert *verts = mesh->verts;
            const Vert *v0 = &verts[hit.index[0]];
            const Vert *v1 = &verts[hit.index[1]];
            const Vert *v2 = &verts[hit.index[2]];

            // geometric normal
            gn = (v1->xyz - v0->xyz).Cross(v2->xyz - v0->xyz);
            //          gn.Normalize();
            gn = shape->axis * gn;
            gn.Normalize();

            // shading normal
            sn = v0->normal * hit.u + v1->normal * hit.v + v2->normal * hit.w;
            //          sn.Normalize();
            sn = shape->axis * sn;
            sn.Normalize();

            st = v0->st * hit.u + v1->st * hit.v + v2->st * hit.w;
        } else {
            material = shape->material;
            n = shape->axis * n;
            sn = n;
            gn = n;
            st = BE1::Vec2(0.0f, 0.0f);
        }

        if (dist > 0.0f && dist < minDist) {
            minDist = dist;
            dg->x = shape->axis * (localStart + localDir * dist) + shape->position;
            dg->sn = sn;
            dg->gn = gn;
            dg->st = st;
            dg->material = material;
        }
    }

    if (minDist < FLT_MAX) {
        return true;
    }

    return false;
}

bool PathTracer::LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const {
    //numRays++;

    for (unsigned int i = 0; i < scene.GetShapes().Count(); i++) {
        const Shape *shape = &scene.GetShapes()[i];

        BE1::Vec3 localStart = shape->axis.TransposedMulVec(start - shape->position);
        BE1::Vec3 localEnd = shape->axis.TransposedMulVec(end - shape->position);

        if (shape->type == Shape::PLANE && shape->plane.IsIntersectLine(localStart, localEnd)) {
            return false;
        }

        if (shape->type == Shape::SPHERE && shape->sphere.IsIntersectLine(localStart, localEnd)) {
            return false;
        }

        if (shape->type == Shape::MODEL && shape->model->LineIntersection(localStart, localEnd)) {
            return false;
        }
    }

    return true;
}

BE1::Vec3 PathTracer::DirectLighting(const BE1::Vec3 &wo, const DifferentialGeometry &dg, int depth) const {
    BE1::Vec3 lightPos;
    BE1::Vec3 wi;
    BE1::Vec3 color = BE1::Vec3(0, 0, 0);
    BE1::Vec3 x2 = dg.x + dg.gn * epsilon;

    for (unsigned int i = 0; i < scene.GetLights().Count(); i++) {
        const Light *light = &scene.GetLights()[i];

        if (light->lightVec.w == 1) {
            // Point light
            lightPos = light->lightVec.ToVec3();
            wi = lightPos - x2;
            wi.Normalize();
        } else {
            // Directional light
            wi = light->lightVec.ToVec3();
        }

        BSDF bsdf;
        dg.material->GetBSDF(dg, &bsdf);
        BE1::Vec3 f = bsdf.Eval(BxDF::All, dg, wi, wo);
        if (f.IsZero()) {
            continue;
        }

        // Test shadow ray intersection
        if (light->lightVec.w == 1) {
            bool v = LineIntersection(x2, lightPos);
            if (v) {
                float rsq = x2.DistanceSqr(lightPos);
                float att = light->intensity * light->intensity / rsq;
                color += light->lightColor * f * att;
            }
        } else {
            //DifferentialGeometry temp;
            //bool v = RayNearestIntersection(x2, lightDir, &temp);
            bool v = LineIntersection(x2, x2 + wi * 9999.0f);
            if (v) {
                color += f * light->lightColor;
            }
        }
    }

    int numShadowRays = 1;
    if (depth == 0) {
        numShadowRays = shadowSampleCount;
    }

    for (unsigned int i = 0; i < scene.GetShapes().Count(); i++) {
        const Shape *shape = &scene.GetShapes()[i];

        if (shape->type == Shape::MODEL) {
            for (int j = 0; j < shape->model->NumMeshes(); j++) {
                const SubMesh *mesh = shape->model->GetMesh(j);
                if (!mesh->material->le.IsZero()) {
                    color += DirectAreaLighting(shape->position, shape->axis, mesh, numShadowRays, dg, x2, wo);
                }
            }
        } else {
            const Material *material = shape->material;
            if (material->le.IsZero()) {
                continue;
            }
        }
    }

    return color;
}

BE1::Vec3 PathTracer::DirectAreaLighting(const BE1::Vec3 &position, const BE1::Mat3 &axis, const SubMesh *mesh, int numShadowRays, const DifferentialGeometry &dg, const BE1::Vec3 &x, const BE1::Vec3 &wo) const {
    // Assume light mesh is a quad
    BE1::Vec3 vec0 = mesh->verts[1].xyz - mesh->verts[0].xyz;
    BE1::Vec3 vec1 = mesh->verts[2].xyz - mesh->verts[0].xyz;
    BE1::Vec3 n = vec0.Cross(vec1);
    BE1::Vec3 wi;
    BE1::Vec3 lightPos;
    BE1::Vec2 u;
    BE1::Vec3 color = BE1::Vec3(0.0f);

    const float emitterArea = n.Length();
    if (emitterArea <= 0.0f || !BE1::Math::IsFinite(emitterArea)) {
        return color;
    }
    const float areaPdf = 1.0f / emitterArea;
    n = axis * (n * areaPdf);

    BSDF bsdf;
    dg.material->GetBSDF(dg, &bsdf);

    for (int j = 0; j < numShadowRays; j++) {
        if (numShadowRays > 1) {
            u = shadowSampler.GetSample(j, rng);
        } else {
            u.x = rng.GetNextFloat();
            u.y = rng.GetNextFloat();
        }

        lightPos = mesh->verts[0].xyz + u.x * vec0 + u.y * vec1;
        lightPos += mesh->verts[0].normal * epsilon;
        lightPos = axis * lightPos + position;

        wi = lightPos - x;
        wi.Normalize();

        BE1::Vec3 f = bsdf.Eval(BxDF::All & ~BxDF::Specular, dg, wi, wo);
        if (f.IsZero()) {
            continue;
        }

        if (LineIntersection(x, lightPos)) {
            float rsq = x.DistanceSqr(lightPos);
            float NdotL2 = n.Dot(-wi);
            float g = NdotL2 / rsq;
            if (NdotL2 > 0) {
                color += mesh->material->le * f * g / areaPdf;
            }
        }
    }

    return color / (float)numShadowRays;
}

// exitant radiance = Le + 1/N * SIGMA BxDF(wi, wo, n) * Li * NdotL / PDF
BE1::Vec3 PathTracer::IndirectLighting(const BE1::Vec3 &wo, const DifferentialGeometry &dg, float invAbsorption, float accumFactor, int depth) const {
    BE1::Vec3 color = BE1::Vec3(0, 0, 0);
    BE1::Vec3 wi;
    BE1::Vec3 f;

    BSDF bsdf;
    dg.material->GetBSDF(dg, &bsdf);

    int numSamples = 1;
    if (depth == 0) {
        //      if (bsdf.NumBxDFs() > 1 || !bsdf[0]->CheckType(BxDF::Specular)) {
        numSamples = splitSampleCount;
        //      }
    }

    for (int s = 0; s < numSamples; s++) {
        // Get random sample direction from material PDF.
        BE1::Vec2 u;
        if (numSamples > 1) {
            u = splitSampler.GetSample(s, rng);
        } else {
            u.x = rng.GetNextFloat();
            u.y = rng.GetNextFloat();
        }

        float pdf = 0.0f;
        int type = 0;
        if (options.importanceSampling) {
            f = bsdf.SampleEval(BxDF::All, dg, wo, u, rng, &wi, &pdf, &type);
        } else {
            BE1::Vec3 sampleDir = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
            wi = RotateWithUpVector(sampleDir, dg.sn);
            f = bsdf.Eval(BxDF::All, dg, wi, wo);
            pdf = BE1::SamplingPDF::CosineHemisphere(dg.sn.Dot(wi));
        }

        if (pdf > 0.0f && !f.IsZero()) {
            // Monte Carlo integrand except for input radiance.
            f /= pdf;

            // Translate ray position slightly according to geometry normal
            BE1::Vec3 offset = dg.gn * epsilon;
            BE1::Vec3 x2 = dg.x + (dg.sn.Dot(wi) > 0.0f ? offset : -offset);

            // Compute incident radiance with wi recursively.
            float luminance = f.x * 0.2989f + f.y * 0.5866f + f.z * 0.1145f;
            const bool specularBounce = (type & BxDF::Specular) != 0;
            BE1::Vec3 li = Li(x2, wi, luminance * accumFactor, depth + 1, specularBounce);

            // Compute exitant radiance
            color += f * li;
        }
    }

    color /= (float)numSamples;
    color *= invAbsorption;
    return color;
}

float PathTracer::AmbientOcclusion(const DifferentialGeometry &dg) const {
    float occ = 0.0f;
    int numSamples = splitSampleCount;

    BE1::Vec3 x2 = dg.x + dg.gn * epsilon;
    BE1::Vec3 dir;

    for (int i = 0; i < numSamples; i++) {
        BE1::Vec2 u = splitSampler.GetSample(i, rng);
        dir = BE1::Vec3::FromCosineSampleHemisphere(u.x, u.y);
        BE1::Vec3 dir2 = RotateWithUpVector(dir, dg.sn);

        if (!LineIntersection(x2, x2 + dir2 * rng.GetNextFloat(minAmbientOcclusionDistance, maxAmbientOcclusionDistance) * unitMeter)) {
            occ += 1.0f;
        }
    }

    return 1.0f - occ / numSamples;
}

BE1::Vec3 PathTracer::Li(const BE1::Vec3 &x, const BE1::Vec3 &dir, float accumFactor, int depth, bool specularBounce) const {
    BE1::Vec3 color = BE1::Vec3(0, 0, 0);
    float absorption = 0.0f;
    float invAbsorption = 1.0f;

    if (depth >= maxDepth) {
        return color;
    }

    // If a accumFactor < 0.01 or russian roulette depth, we stop recursion with probability of 50%
    if (depth >= minDepth) {
        if (depth >= russianRouletteDepth || accumFactor < 0.01f) {
            absorption = 0.5f; // absorption probability
            invAbsorption = 1.0f / (1.0f - absorption);

            if (rng.GetNextFloat() < absorption) {
                return color;
            }
        }
    }

    if (options.ambientOcclusion && depth == 1) {
        if (!scene.GetLightProbeTexture()) {
            return BE1::Vec3(0.1f); // ambient color
        }
        return scene.GetLightProbeTexture()->Sample(dir).ToVec3();
    }

    DifferentialGeometry dg;
    if (!RayNearestIntersection(x, dir, &dg)) {
        if (!scene.GetLightProbeTexture()) {
            //color.Lerp(BE1::Vec3(0.16f, 0.33f, 0.61f), BE1::Vec3(0.5f, 0.7f, 0.9f), powf(acosf(dir.z) / Math::HALF_PI, 10.0f));
            return color;
        }
        return scene.GetLightProbeTexture()->Sample(dir).ToVec3();
    }

    // Compute emitted radiance
    if (!options.directLighting || depth == 0 || specularBounce) {
        // NOTE: 비-specular 경로의 Le 는 direct lighting 항에서 계산하고, specular 경로의 Le 는 여기서 계산한다.
        color += dg.material->le * invAbsorption;
        if (!dg.material->le.IsZero()) {
            return color;
        }
    }

    // Compute direct radiance
    if (options.directLighting) {
        color += DirectLighting(-dir, dg, depth) * invAbsorption;
    }

    // Compute indirect radiance
    if (options.indirectLighting) {
        BE1::Vec3 idc = IndirectLighting(-dir, dg, invAbsorption, accumFactor, depth);
        // Compute ambient occlusion
        if (options.ambientOcclusion && depth == 0 && idc != BE1::Vec3::zero) {
            idc *= AmbientOcclusion(dg);
        }

        color += idc;
    }

    return color;
}
