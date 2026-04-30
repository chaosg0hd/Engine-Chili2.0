#include "engine/game_runtime_api.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <cstring>
#include <iostream>

namespace
{
    constexpr const char* kGameRuntimeDllName = "pong_runtime.dll";
}

int main()
{
    HMODULE gameRuntime = LoadLibraryA(kGameRuntimeDllName);
    if (!gameRuntime)
    {
        std::cerr << "GamePreview: failed to load " << kGameRuntimeDllName << ".\n";
        return 1;
    }

    FARPROC rawGetGameApi = GetProcAddress(gameRuntime, "get_game_api");
    if (!rawGetGameApi)
    {
        std::cerr << "GamePreview: get_game_api was not exported by " << kGameRuntimeDllName << ".\n";
        FreeLibrary(gameRuntime);
        return 1;
    }

    GetGameApiFn getGameApi = nullptr;
    static_assert(sizeof(getGameApi) == sizeof(rawGetGameApi), "Function pointer size mismatch.");
    std::memcpy(&getGameApi, &rawGetGameApi, sizeof(getGameApi));

    const GameRuntimeApi* api = getGameApi();
    if (!api || api->apiVersion != 1U || !api->runPreview)
    {
        std::cerr << "GamePreview: incompatible GameRuntime API.\n";
        FreeLibrary(gameRuntime);
        return 1;
    }

    std::cout << "GamePreview: running " << (api->runtimeName ? api->runtimeName : "GameRuntime") << ".\n";
    const int result = api->runPreview();
    FreeLibrary(gameRuntime);
    return result;
}
