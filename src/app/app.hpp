#pragma once

class EngineCore;

class App
{
public:
    bool Run();

private:
    bool RunStartupChecks(EngineCore& core);
    bool RunMemoryFeatureTest(EngineCore& core);
    bool RunJobFeatureTest(EngineCore& core);
    void LogFeatureSummary(EngineCore& core) const;
};
