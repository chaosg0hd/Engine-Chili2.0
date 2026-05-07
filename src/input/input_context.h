#pragma once

#include "input_action_state.h"
#include "input_binding.h"

#include <string>
#include <unordered_map>
#include <vector>

class InputContext
{
public:
    explicit InputContext(std::string name = std::string(), int priority = 0)
        : m_name(std::move(name)),
          m_priority(priority)
    {
    }

    const std::string& GetName() const { return m_name; }
    int GetPriority() const { return m_priority; }
    void SetPriority(int priority) { m_priority = priority; }

    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    void BindAction(const std::string& actionName, const InputBinding& binding)
    {
        m_bindings[actionName].push_back(binding);
    }

    const std::unordered_map<std::string, std::vector<InputBinding>>& GetBindings() const
    {
        return m_bindings;
    }

    void ClearActionStates()
    {
        for (auto& entry : m_actionStates)
        {
            entry.second = InputActionState{};
        }
    }

    void SetActionState(const std::string& actionName, const InputActionState& state)
    {
        m_actionStates[actionName] = state;
    }

    const InputActionState& GetActionState(const std::string& actionName) const
    {
        const auto it = m_actionStates.find(actionName);
        static const InputActionState kEmpty{};
        return it != m_actionStates.end() ? it->second : kEmpty;
    }

private:
    std::string m_name;
    int m_priority = 0;
    bool m_enabled = true;
    std::unordered_map<std::string, std::vector<InputBinding>> m_bindings;
    std::unordered_map<std::string, InputActionState> m_actionStates;
};
