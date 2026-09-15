#pragma once

#include "Render/Renderer.h"
#include "Scene/Scene.h"
#include "Scene/ScenePresets.h"
#include <atomic>
#include <vector>

struct RenderStats {
    int                             elapsedMsec = 0;
    int                             completedSamples = 0;
    int                             targetSamples = 0;
    uint64_t                        tracedRays = 0;
};

enum class ScreenshotResult {
    Saved,
    WriteFailed,
    NamesExhausted
};

class App {
public:
    App &                           operator=(const App &) = delete;

    void                            Init(BE1::TaskManager &taskScheduler, int width, int height, const Options &options = {}, ScenePreset preset = ScenePreset::Default);
    void                            Shutdown();

    bool                            Start(const Options &options);
    void                            Stop();
    bool                            Update();

    bool                            IsRendering() const { return workInProgress.load(std::memory_order_relaxed); }
    const RenderStats &             GetStats() const { return stats; }
    const DIB &                     GetImage() const { return renderer.GetImage(); }

    ScreenshotResult                TakeScreenshot();

private:
    struct TileInfo {
        App *app;
        int x1, x2;
        int y1, y2;
    };

    static void                     TaskProc(void *param);
    bool                            SubmitRenderPass();
    void                            UpdateStats();

    BE1::TaskManager *              taskScheduler = nullptr;
    SceneManager                    scene;
    SceneView                       view;
    Renderer                        renderer;
    std::vector<TileInfo>           tiles;

    // UI edits are copied here before starting tasks; workers only read this snapshot.
    Options                         renderOptions;
    RenderStats                     stats;
    std::atomic<bool>               workInProgress{false};
    std::atomic<uint64_t>           numTotalRays{0};
    int                             currentPass = 0;
    int                             startTime = 0;
};
