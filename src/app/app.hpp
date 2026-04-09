#pragma once

class EngineCore;

class App
{
public:
    bool Run();

private:
    void UpdateFrame(EngineCore& core);
};
