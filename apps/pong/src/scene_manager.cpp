#include "scene_manager.hpp"

#include <utility>

void SceneManager::AddScene(int index, std::string path)
{
    m_scenes[index] = std::move(path);
}

void SceneManager::RequestTransition(int sceneIndex)
{
    if (m_scenes.count(sceneIndex))
        m_pendingTransition = sceneIndex;
}

void SceneManager::RequestExit()
{
    m_exitPending = true;
}

int SceneManager::ConsumeTransition()
{
    const int t    = m_pendingTransition;
    m_pendingTransition = -1;
    return t;
}

bool SceneManager::ConsumeExit()
{
    const bool e = m_exitPending;
    m_exitPending = false;
    return e;
}

const std::string& SceneManager::GetScenePath(int index) const
{
    static const std::string s_empty;
    const auto it = m_scenes.find(index);
    return it != m_scenes.end() ? it->second : s_empty;
}

int SceneManager::GetCurrentScene() const
{
    return m_currentScene;
}

void SceneManager::SetCurrentScene(int index)
{
    m_currentScene = index;
}

bool SceneManager::HasScene(int index) const
{
    return m_scenes.count(index) > 0;
}
