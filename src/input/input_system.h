#pragma once

#include "input_context.h"
#include "raw_input_state.h"

#include <string>
#include <unordered_map>
#include <vector>

struct InputVec2
{
    float x = 0.0f;
    float y = 0.0f;
};

class InputSystem
{
public:
    InputContext& RegisterContext(const std::string& name, int priority = 0);
    InputContext* FindContext(const std::string& name);
    const InputContext* FindContext(const std::string& name) const;

    void SetContextEnabled(const std::string& name, bool enabled);
    void Evaluate(const RawInputState& rawInput);

    bool Pressed(const std::string& contextName, const std::string& actionName) const;
    bool Down(const std::string& contextName, const std::string& actionName) const;
    bool Released(const std::string& contextName, const std::string& actionName) const;
    float Value(const std::string& contextName, const std::string& actionName) const;
    InputVec2 Delta(const std::string& contextName, const std::string& actionName) const;

    InputVec2 MousePosition() const;
    InputVec2 MouseDelta() const;
    float MouseWheelDelta() const;

    std::vector<std::string> GetActiveContextsByPriority() const;

private:
    bool BindingModifiersMatch(const InputBinding& binding, const RawInputState& input) const;

private:
    RawInputState m_rawInput;
    std::unordered_map<std::string, InputContext> m_contexts;
};
