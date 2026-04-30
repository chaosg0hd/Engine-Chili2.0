#pragma once

#include <cstdint>

#if defined(_WIN32)
#if defined(GAME_RUNTIME_EXPORTS)
#define GAME_RUNTIME_API_EXPORT __declspec(dllexport)
#else
#define GAME_RUNTIME_API_EXPORT __declspec(dllimport)
#endif
#else
#define GAME_RUNTIME_API_EXPORT
#endif

extern "C"
{
    struct GameRuntimeApi
    {
        std::uint32_t apiVersion;
        const char* runtimeName;

        // Temporary preview entry point. TODO: replace with create/tick/destroy
        // callbacks once Studio and GamePreview host app DLLs directly.
        int (*runPreview)();
    };

    using GetGameApiFn = const GameRuntimeApi* (*)();

    GAME_RUNTIME_API_EXPORT const GameRuntimeApi* get_game_api();
}
