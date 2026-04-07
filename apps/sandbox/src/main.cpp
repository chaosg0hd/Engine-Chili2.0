#include "sandbox_app.hpp"

int main()
{
    SandboxApp app;
    return app.Run() ? 0 : 1;
}
