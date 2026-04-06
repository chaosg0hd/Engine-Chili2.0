#include "app/app.hpp"

int main()
{
    App app;

    if (!app.Run())
    {
        return 1;
    }

    return 0;
}