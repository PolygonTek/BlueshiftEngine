#include "Precompiled.h"
#include "Render/Renderer.h"
#include "Render/DIB.h"
#include "Render/Camera.h"
#include "Render/PathTracer.h"
#include "Render/Random.h"
#include "Render/Sampler.h"

/*static float poissonDiscKernel[12][2] = {
    { -0.326212f, -0.405805f },
    { -0.840144f, -0.073580f },
    { -0.695914f, +0.457137f },
    { -0.203345f, +0.620716f },
    { +0.962340f, -0.194983f },
    { +0.473434f, -0.480026f },
    { +0.519456f, +0.767022f },
    { +0.185461f, -0.893124f },
    { +0.507431f, +0.064425f },
    { +0.896420f, +0.412458f },
    { -0.321940f, -0.932615f },
    { -0.791559f, -0.597705f }
};*/

Renderer::~Renderer() {
    Shutdown();
}

void Renderer::Init(int screenWidth, int screenHeight, const Options &options) {
    Shutdown();

    this->options = options;
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;

    dib.Create(screenWidth, screenHeight);

    invGamma = 1.0f / 2.2f;

    floatBuffer = new float[screenWidth * screenHeight * 3];
    memset(floatBuffer, 0, sizeof(float) * screenWidth * screenHeight * 3);

    superSampleKernel = nullptr;
    InitSuperSample(options.maxSamplePasses);

    lensSampleCount = BE1::Max(64 / options.maxSamplePasses, 1);
    BE1::RNG rng(MakeRenderSeed(options.randomSeed, 1));
    lensSampler.Init(lensSampleCount, rng.GetNextUInt());
    for (int i = 0; i < lensSampleCount; ++i) {
        lensSamples[i] = lensSampler.GetSample(i, rng);
    }
}

void Renderer::Shutdown() {
    if (superSampleKernel) {
        delete[] superSampleKernel;
    }

    superSampleKernel = nullptr;
    delete[] floatBuffer;
    floatBuffer = nullptr;

    superSampler.Clear();
    lensSampler.Clear();
    lensSampleCount = 0;
    dib.Destroy();
    screenWidth = screenHeight = 0;
}

void Renderer::Reset(const Options &options) {
    this->options = options;
    memset(floatBuffer, 0, sizeof(float) * screenWidth * screenHeight * 3);
    dib.Fill(BE1::Vec3(0, 0, 0));

    InitSuperSample(options.maxSamplePasses);

    lensSampleCount = BE1::Max(64 / options.maxSamplePasses, 1);
    BE1::RNG rng(MakeRenderSeed(options.randomSeed, 1));
    lensSampler.Init(lensSampleCount, rng.GetNextUInt());
    for (int i = 0; i < lensSampleCount; ++i) {
        lensSamples[i] = lensSampler.GetSample(i, rng);
    }
}

void Renderer::InitSuperSample(int superSampleCount) {
    if (superSampleKernel) {
        delete[] superSampleKernel;
        superSampleKernel = nullptr;
    }

    if (superSampleCount > 1) {
        BE1::RNG rng(MakeRenderSeed(options.randomSeed, 0));
        superSampler.Init(superSampleCount, rng.GetNextUInt());
        superSampleKernel = new BE1::Vec2[superSampleCount];
        for (int i = 0; i < superSampleCount; i++) {
            superSampleKernel[i] = BE1::Vec2(0.5f, 0.5f) - superSampler.GetSample(i, rng);
        }
    }
}

void Renderer::Rasterize(const PathTracer *tracer, const PinholeCamera &camera, int k, int y, int x1, int x2) {
    float invScreenWidth = 1.0f / screenWidth;
    float invScreenHeight = 1.0f / screenHeight;
    float invK = 1.0f / k;

    int numSamples = 1;
    float invNumSamples = 1.0f / numSamples;

    for (int x = x1; x < x2; x++) {
        BE1::Vec3 color = BE1::Vec3(0.0f);

        for (int i = 0; i < numSamples; i++) {
            BE1::Vec2 jitter = BE1::Vec2(0, 0);
            if (options.maxSamplePasses > 1) {
                jitter = superSampleKernel[k - 1];
            }

            float ndx = ((x + jitter.x) * invScreenWidth) * 2.0f - 1.0f;
            float ndy = 1.0f - ((y + jitter.y) * invScreenHeight) * 2.0f;
            BE1::Vec3 p, dir;

            if (!options.useDOF) {
                camera.GetRay(ndx, ndy, &p, &dir);
                color += tracer->Li(p, dir, 1.0f, 0) * invNumSamples;
            } else {
                BE1::Vec3 c = BE1::Vec3(0.0f);

                for (int i = 0; i < lensSampleCount; i++) {
                    BE1::Vec2 u = lensSamples[i];
                    BE1::Vec2 diskSample = BE1::Vec2::FromConcentricSampleDisk(u.x, u.y);

                    camera.GetPinholeRay(ndx, ndy, diskSample.x, diskSample.y, &p, &dir);
                    c += tracer->Li(p, dir, 1.0f, 0);
                }

                c /= (float)lensSampleCount;
                color += c * invNumSamples;
            }
        }

        BE1::Vec3 *destColor = reinterpret_cast<BE1::Vec3 *>(&floatBuffer[(y * screenWidth + x) * 3]);
        *destColor += color;

        BE1::Vec3 finalColor = *destColor * invK;

        // gamma correction
        finalColor.x = powf(finalColor.x, invGamma);
        finalColor.y = powf(finalColor.y, invGamma);
        finalColor.z = powf(finalColor.z, invGamma);

        dib.SetColor(x, y, finalColor);
    }
}
