#pragma once

struct AppCapabilities;

class App
{
public:
    bool Run();

private:
    void UpdateFrame(AppCapabilities& capabilities);
};
