#pragma once

#include <random>

class EngineCore;

class App
{
public:
    bool Run();

private:
    void RunPixelBlinkTest(EngineCore& core);
    bool RunStartupChecks(EngineCore& core);
    bool RunMemoryFeatureTest(EngineCore& core);
    bool RunRawMemoryFeatureTest(EngineCore& core);
    bool RunFileFeatureTest(EngineCore& core);
    bool RunGpuFeatureTest(EngineCore& core);
    bool RunJobFeatureTest(EngineCore& core);
    bool RunInputFeatureTest(EngineCore& core);
    void UpdateRollGame(EngineCore& core);
    void LogFeatureSummary(EngineCore& core) const;

private:
    std::mt19937 m_rng{std::random_device{}()};
    int m_score = 0;
    int m_lastRoll = 0;
    unsigned long long m_totalRolls = 0;
    unsigned long long m_tenHits = 0;
    double m_rollTimer = 0.0;
    bool m_paused = false;
    bool m_pixelTestEnabled = false;
};
