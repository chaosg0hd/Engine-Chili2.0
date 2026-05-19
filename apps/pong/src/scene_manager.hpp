#pragma once

#include <string>
#include <unordered_map>

// Reusable scene registry and transition signal bus.
// Game hosts AddScene() entries, then poll ConsumeTransition() / ConsumeExit() each frame.
class SceneManager
{
public:
    void AddScene(int index, std::string path);

    void RequestTransition(int sceneIndex);
    void RequestExit();

    int  ConsumeTransition(); // returns -1 if no transition pending
    bool ConsumeExit();

    const std::string& GetScenePath(int index) const;
    int  GetCurrentScene() const;
    void SetCurrentScene(int index);
    bool HasScene(int index) const;

private:
    std::unordered_map<int, std::string> m_scenes;
    int  m_currentScene      = -1;
    int  m_pendingTransition = -1;
    bool m_exitPending       = false;
};
