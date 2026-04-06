#include "console.hpp"

#include <windows.h>

namespace Console
{
    void Clear()
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        if (hConsole == nullptr || hConsole == INVALID_HANDLE_VALUE)
        {
            return;
        }

        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
        {
            return;
        }

        const DWORD cellCount =
            static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);

        DWORD written = 0;
        COORD home = { 0, 0 };

        FillConsoleOutputCharacterA(hConsole, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(hConsole, home);
    }
}