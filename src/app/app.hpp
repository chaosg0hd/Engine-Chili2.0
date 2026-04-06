#pragma once

class EngineCore;

class App
{
public:
    bool Run();

private:
    bool RunStartupChecks(EngineCore& core);
    bool RunMemoryFeatureTest(EngineCore& core);
    bool RunRawMemoryFeatureTest(EngineCore& core);
    bool RunJobFeatureTest(EngineCore& core);
    bool RunInputFeatureTest(EngineCore& core);
    void LogFeatureSummary(EngineCore& core) const;
};
