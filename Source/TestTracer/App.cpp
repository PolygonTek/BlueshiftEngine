#include "Precompiled.h"
#include "App.h"
#include "Render/PathTracer.h"
#include "Render/Random.h"
#include <io.h>

static constexpr int TileSize = 64;

void App::Init(BE1::TaskManager &taskScheduler, int width, int height, const Options &options, ScenePreset preset) {
    Shutdown();

    if (width <= 0 || height <= 0 || !options.IsValid()) {
        return;
    }

    this->taskScheduler = &taskScheduler;

    renderOptions = options;
    renderer.Init(width, height, renderOptions);

    BuildScene(scene, view, width, height, preset);

    tiles.reserve(((width + TileSize - 1) / TileSize) * ((height + TileSize - 1) / TileSize));
    for (int y = 0; y < height; y += TileSize) {
        for (int x = 0; x < width; x += TileSize) {
            tiles.push_back({this, x, BE1::Min(x + TileSize, width), y, BE1::Min(y + TileSize, height)});
        }
    }
}

void App::Shutdown() {
    Stop();

    taskScheduler = nullptr;
    tiles.clear();
    renderer.Shutdown();
    scene.Shutdown();
    currentPass = 0;
    stats = {};
}

bool App::Start(const Options &options) {
    if (!taskScheduler || !options.IsValid()) {
        return false;
    }

    Stop();

    renderOptions = options;
    renderer.Reset(renderOptions);
    numTotalRays.store(0, std::memory_order_relaxed);
    stats = {};
    stats.targetSamples = renderOptions.maxSamplePasses * renderOptions.splitSampleCount;
    currentPass = 1;
    startTime = BE1::PlatformTime::Milliseconds();
    workInProgress.store(true, std::memory_order_relaxed);
    return SubmitRenderPass();
}

void App::Stop() {
    const bool wasRendering = workInProgress.exchange(false, std::memory_order_relaxed);
    if (taskScheduler) {
        taskScheduler->WaitFinishAll();
    }
    if (wasRendering) {
        UpdateStats();
    }
}

bool App::Update() {
    if (!IsRendering()) {
        return false;
    }

    if (taskScheduler->TimedWaitFinishAll(0)) {
        stats.completedSamples = currentPass * renderOptions.splitSampleCount;
        if (currentPass < renderOptions.maxSamplePasses) {
            ++currentPass;
            SubmitRenderPass();
        } else {
            workInProgress.store(false, std::memory_order_relaxed);
        }
    }

    UpdateStats();
    return true;
}

void App::UpdateStats() {
    stats.elapsedMsec = BE1::PlatformTime::Milliseconds() - startTime;
    stats.tracedRays = numTotalRays.load(std::memory_order_relaxed);
}

void App::TaskProc(void *param) {
    const TileInfo &tile = *static_cast<const TileInfo *>(param);
    App &app = *tile.app;
    if (!app.IsRendering()) {
        return;
    }

    // Each worker reuses its sample tables across tiles and render passes.
    thread_local Sampler splitSampler;
    thread_local Sampler shadowSampler;
    splitSampler.Init(app.renderOptions.splitSampleCount, MakeRenderSeed(app.renderOptions.randomSeed, 2));
    shadowSampler.Init(app.renderOptions.shadowSampleCount, MakeRenderSeed(app.renderOptions.randomSeed, 3));

    // Tile identity, not worker assignment, determines the path's random stream.
    const uint64_t tileIndex = static_cast<uint64_t>(&tile - app.tiles.data());
    const uint64_t stream = 4 + static_cast<uint64_t>(app.currentPass - 1) * app.tiles.size() + tileIndex;
    PathTracer tracer(app.scene, app.renderOptions, app.view.unitMeter, splitSampler, shadowSampler,
        MakeRenderSeed(app.renderOptions.randomSeed, stream));
    for (int y = tile.y1; y < tile.y2; ++y) {
        app.renderer.Rasterize(&tracer, app.view.camera, app.currentPass, y, tile.x1, tile.x2);
        if (!app.IsRendering()) {
            break;
        }
    }
    app.numTotalRays.fetch_add(tracer.NumTracedRays(), std::memory_order_relaxed);
}

bool App::SubmitRenderPass() {
    // The tile storage and pass settings stay stable until every task has finished.
    for (TileInfo &tile : tiles) {
        if (!taskScheduler->AddTask(&TaskProc, &tile)) {
            Stop();
            return false;
        }
    }
    return true;
}

ScreenshotResult App::TakeScreenshot() {
    if (!taskScheduler) {
        return ScreenshotResult::WriteFailed;
    }

    // Finish this pass before reading the framebuffer.
    taskScheduler->WaitFinishAll();

    BE1::fileSystem.CreateDirectory("screenshots", true);
    wchar_t path[64];
    for (int i = 0; i < 1000; ++i) {
        swprintf_s(path, L"screenshots/shot%03d.png", i);
        if (_waccess(path, 0) != 0) {
            return renderer.GetImage().WritePNG(path) ? ScreenshotResult::Saved : ScreenshotResult::WriteFailed;
        }
    }
    return ScreenshotResult::NamesExhausted;
}
