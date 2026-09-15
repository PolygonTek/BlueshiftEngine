#pragma once

#include "Render/DIB.h"
#include "Render/Sampler.h"
#include "Render/Options.h"

class PathTracer;
class PinholeCamera;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer &) = delete;
    Renderer &                      operator=(const Renderer &) = delete;

    void                            Init(int screenWidth, int screenHeight, const Options &options);
    void                            Shutdown();

    void                            Reset(const Options &options);
    void                            Rasterize(const PathTracer *tracer, const PinholeCamera &camera, int k, int y, int x1, int x2);

    const DIB &                     GetImage() const { return dib; }

private:
    void                            InitSuperSample(int superSampleCount);

    // Written before task submission and unchanged until all tasks finish.
    Options                         options;
    int                             screenWidth = 0;
    int                             screenHeight = 0;
    float                           invGamma = 1.0f / 2.2f;
    DIB                             dib;
    float *                         floatBuffer = nullptr;
    Sampler                         superSampler;
    BE1::Vec2 *                     superSampleKernel = nullptr;
    int                             lensSampleCount = 0;
    Sampler                         lensSampler;
    BE1::Vec2                       lensSamples[64];
};
