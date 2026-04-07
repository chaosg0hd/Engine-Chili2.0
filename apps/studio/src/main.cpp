#include "studio_app.hpp"

int main()
{
    StudioApp app;
    if (!app.Initialize())
    {
        return 1;
    }

    app.Run();
    app.Shutdown();
    return 0;
}
