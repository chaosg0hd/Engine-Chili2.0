#pragma once

#include <random>
#include <string>
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

    struct FeatureCheckResult
    {
        std::string name;
        bool passed = false;
        std::string detail;
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
    bool RunResourceFeatureTest(EngineCore& core);
    bool RunInputFeatureTest(EngineCore& core);
    bool ExecuteFeatureTest(EngineCore& core, const std::string& name, bool (App::*test)(EngineCore&));
    void RecordFeatureResult(EngineCore& core, const std::string& name, bool passed, const std::string& detail = std::string());
    std::wstring BuildFeatureSummaryOverlay() const;
    void LogFeatureResultSummary(EngineCore& core) const;
    void LogFeatureSummary(EngineCore& core) const;

private:
    std::mt19937 m_rng{std::random_device{}()};
    DisplayMode m_displayMode = DisplayMode::TextOverlay;
    int m_starFieldWidth = 0;
    int m_starFieldHeight = 0;
    std::vector<BlinkStar> m_stars;
    std::vector<FeatureCheckResult> m_featureChecks;
};
