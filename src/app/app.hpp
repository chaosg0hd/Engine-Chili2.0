#pragma once

#include <random>
#include <vector>

class EngineCore;

class App
{
public:
    bool Run();

private:
    enum class DisplayMode
    {
        TextOverlay = 0,
        PixelRenderer
    };

    struct BlinkStar
    {
        int x = 0;
        int y = 0;
        bool visible = true;
        double blinkTimer = 0.0;
        double blinkInterval = 0.0;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateDisplayToggle(EngineCore& core);
    void RunTextOverlayMode(EngineCore& core);
    void RunPixelRendererMode(EngineCore& core);
    void RebuildStarField(int width, int height);
    bool RunStartupChecks(EngineCore& core);
    bool RunMemoryFeatureTest(EngineCore& core);
    bool RunRawMemoryFeatureTest(EngineCore& core);
    bool RunFileFeatureTest(EngineCore& core);
    bool RunGpuFeatureTest(EngineCore& core);
    bool RunJobFeatureTest(EngineCore& core);
    bool RunInputFeatureTest(EngineCore& core);
    void LogFeatureSummary(EngineCore& core) const;

private:
    std::mt19937 m_rng{std::random_device{}()};
    DisplayMode m_displayMode = DisplayMode::TextOverlay;
    int m_starFieldWidth = 0;
    int m_starFieldHeight = 0;
    std::vector<BlinkStar> m_stars;
};
