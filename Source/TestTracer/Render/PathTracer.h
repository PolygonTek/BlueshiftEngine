#pragma once

#include "Render/Sampler.h"
#include "Render/Options.h"
#include <cstdint>

class SceneManager;
class SubMesh;
class DifferentialGeometry;
class TextureLightProbe;

class PathTracer {
public:
    PathTracer(const SceneManager &scene, const Options &options, float unitMeter, Sampler &splitSampler, Sampler &shadowSampler, uint64_t seed);

    void                            SetAmbientOcclusionDistance(float mindist, float maxdist);
    uint64_t                        NumTracedRays() const { return numRays; }
    void                            ClearTracedRays() { numRays = 0; }

    BE1::Vec3                       Li(const BE1::Vec3 &x, const BE1::Vec3 &dir, float accumFactor, int depth, bool specularBounce = false) const;

private:
    BE1::Vec3                       DirectLighting(const BE1::Vec3 &wo, const DifferentialGeometry &dg, int depth) const;
    BE1::Vec3                       DirectAreaLighting(const BE1::Vec3 &position, const BE1::Mat3 &axis, const SubMesh *mesh, int numShadowRays, const DifferentialGeometry &dg, const BE1::Vec3 &x, const BE1::Vec3 &wo) const;
    BE1::Vec3                       IndirectLighting(const BE1::Vec3 &wo, const DifferentialGeometry &dg, float invAbsorption, float accumFactor, int depth) const;
    float                           AmbientOcclusion(const DifferentialGeometry &dg) const;
    bool                            RayNearestIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, DifferentialGeometry *dg) const;
    bool                            LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const;

    const SceneManager &            scene;
    const Options                   options;
    int                             minDepth = 1;
    int                             maxDepth = 5;
    int                             russianRouletteDepth = 4;
    int                             splitSampleCount;
    int                             shadowSampleCount;
    Sampler &                       splitSampler;
    Sampler &                       shadowSampler;
    mutable BE1::RNG                rng;
    float                           unitMeter;
    float                           epsilon;
    float                           minAmbientOcclusionDistance = 0.04f;
    float                           maxAmbientOcclusionDistance = 0.6f;
    mutable uint64_t                numRays = 0;
};

BE_INLINE void PathTracer::SetAmbientOcclusionDistance(float mindist, float maxdist) {
    minAmbientOcclusionDistance = mindist;
    maxAmbientOcclusionDistance = maxdist;
}
