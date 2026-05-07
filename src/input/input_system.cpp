#include "input_system.h"

#include <algorithm>
#include <array>

namespace
{
    struct ContextOrder
    {
        int priority = 0;
        std::string name;
    };
}

InputContext& InputSystem::RegisterContext(const std::string& name, int priority)
{
    auto it = m_contexts.find(name);
    if (it == m_contexts.end())
    {
        it = m_contexts.emplace(name, InputContext(name, priority)).first;
    }
    else
    {
        it->second.SetPriority(priority);
    }
    return it->second;
}

InputContext* InputSystem::FindContext(const std::string& name)
{
    const auto it = m_contexts.find(name);
    return it != m_contexts.end() ? &it->second : nullptr;
}

const InputContext* InputSystem::FindContext(const std::string& name) const
{
    const auto it = m_contexts.find(name);
    return it != m_contexts.end() ? &it->second : nullptr;
}

void InputSystem::SetContextEnabled(const std::string& name, bool enabled)
{
    if (InputContext* context = FindContext(name))
    {
        context->SetEnabled(enabled);
    }
}

void InputSystem::Evaluate(const RawInputState& rawInput)
{
    m_rawInput = rawInput;
    for (auto& entry : m_contexts)
    {
        entry.second.ClearActionStates();
    }

    std::vector<ContextOrder> contextOrder;
    contextOrder.reserve(m_contexts.size());
    for (const auto& entry : m_contexts)
    {
        if (entry.second.IsEnabled())
        {
            contextOrder.push_back(ContextOrder{ entry.second.GetPriority(), entry.first });
        }
    }

    std::sort(
        contextOrder.begin(),
        contextOrder.end(),
        [](const ContextOrder& lhs, const ContextOrder& rhs)
        {
            return lhs.priority > rhs.priority;
        });

    std::array<bool, static_cast<std::size_t>(InputKey::Count)> consumedKeys{};
    std::array<bool, static_cast<std::size_t>(InputMouseButton::Count)> consumedMouse{};
    bool consumedMouseWheel = false;

    for (const ContextOrder& ordered : contextOrder)
    {
        InputContext* context = FindContext(ordered.name);
        if (!context)
        {
            continue;
        }

        std::array<bool, static_cast<std::size_t>(InputKey::Count)> consumeKeysThisContext{};
        std::array<bool, static_cast<std::size_t>(InputMouseButton::Count)> consumeMouseThisContext{};
        bool consumeWheelThisContext = false;

        const auto& bindings = context->GetBindings();
        for (const auto& actionEntry : bindings)
        {
            const std::string& actionName = actionEntry.first;
            const std::vector<InputBinding>& actionBindings = actionEntry.second;
            InputActionState actionState;

            for (const InputBinding& binding : actionBindings)
            {
                if (!BindingModifiersMatch(binding, m_rawInput))
                {
                    continue;
                }

                if (binding.kind == InputBindingKind::Key)
                {
                    const std::size_t keyIndex = static_cast<std::size_t>(binding.key);
                    if (keyIndex >= consumedKeys.size() || consumedKeys[keyIndex])
                    {
                        continue;
                    }

                    const RawButtonState& keyState = m_rawInput.Key(binding.key);
                    actionState.pressed = actionState.pressed || keyState.pressed;
                    actionState.down = actionState.down || keyState.down;
                    actionState.released = actionState.released || keyState.released;
                    actionState.value = actionState.down ? 1.0f : actionState.value;

                    if (binding.consume && (keyState.pressed || keyState.down || keyState.released))
                    {
                        consumeKeysThisContext[keyIndex] = true;
                    }
                }
                else if (binding.kind == InputBindingKind::MouseButton)
                {
                    const std::size_t buttonIndex = static_cast<std::size_t>(binding.mouseButton);
                    if (buttonIndex >= consumedMouse.size() || consumedMouse[buttonIndex])
                    {
                        continue;
                    }

                    const RawButtonState& buttonState = m_rawInput.Mouse(binding.mouseButton);
                    actionState.pressed = actionState.pressed || buttonState.pressed;
                    actionState.down = actionState.down || buttonState.down;
                    actionState.released = actionState.released || buttonState.released;
                    actionState.value = actionState.down ? 1.0f : actionState.value;
                    if (actionState.down)
                    {
                        actionState.deltaX = static_cast<float>(m_rawInput.mouseDeltaX);
                        actionState.deltaY = static_cast<float>(m_rawInput.mouseDeltaY);
                    }

                    if (binding.consume && (buttonState.pressed || buttonState.down || buttonState.released))
                    {
                        consumeMouseThisContext[buttonIndex] = true;
                    }
                }
                else if (binding.kind == InputBindingKind::MouseWheel)
                {
                    if (consumedMouseWheel)
                    {
                        continue;
                    }

                    const bool wheelMoved = m_rawInput.mouseWheelDelta != 0;
                    if (wheelMoved)
                    {
                        actionState.pressed = true;
                        actionState.down = true;
                        actionState.released = true;
                        actionState.value = static_cast<float>(m_rawInput.mouseWheelDelta);
                        actionState.deltaY = static_cast<float>(m_rawInput.mouseWheelDelta);
                    }

                    if (binding.consume && wheelMoved)
                    {
                        consumeWheelThisContext = true;
                    }
                }
            }

            context->SetActionState(actionName, actionState);
        }

        for (std::size_t keyIndex = 0; keyIndex < consumeKeysThisContext.size(); ++keyIndex)
        {
            consumedKeys[keyIndex] = consumedKeys[keyIndex] || consumeKeysThisContext[keyIndex];
        }
        for (std::size_t buttonIndex = 0; buttonIndex < consumeMouseThisContext.size(); ++buttonIndex)
        {
            consumedMouse[buttonIndex] = consumedMouse[buttonIndex] || consumeMouseThisContext[buttonIndex];
        }
        consumedMouseWheel = consumedMouseWheel || consumeWheelThisContext;
    }
}

bool InputSystem::Pressed(const std::string& contextName, const std::string& actionName) const
{
    const InputContext* context = FindContext(contextName);
    return context ? context->GetActionState(actionName).pressed : false;
}

bool InputSystem::Down(const std::string& contextName, const std::string& actionName) const
{
    const InputContext* context = FindContext(contextName);
    return context ? context->GetActionState(actionName).down : false;
}

bool InputSystem::Released(const std::string& contextName, const std::string& actionName) const
{
    const InputContext* context = FindContext(contextName);
    return context ? context->GetActionState(actionName).released : false;
}

float InputSystem::Value(const std::string& contextName, const std::string& actionName) const
{
    const InputContext* context = FindContext(contextName);
    return context ? context->GetActionState(actionName).value : 0.0f;
}

InputVec2 InputSystem::Delta(const std::string& contextName, const std::string& actionName) const
{
    const InputContext* context = FindContext(contextName);
    if (!context)
    {
        return InputVec2{};
    }

    const InputActionState& action = context->GetActionState(actionName);
    return InputVec2{ action.deltaX, action.deltaY };
}

InputVec2 InputSystem::MousePosition() const
{
    return InputVec2{ static_cast<float>(m_rawInput.mouseX), static_cast<float>(m_rawInput.mouseY) };
}

InputVec2 InputSystem::MouseDelta() const
{
    return InputVec2{ static_cast<float>(m_rawInput.mouseDeltaX), static_cast<float>(m_rawInput.mouseDeltaY) };
}

float InputSystem::MouseWheelDelta() const
{
    return static_cast<float>(m_rawInput.mouseWheelDelta);
}

std::vector<std::string> InputSystem::GetActiveContextsByPriority() const
{
    std::vector<ContextOrder> ordered;
    ordered.reserve(m_contexts.size());
    for (const auto& entry : m_contexts)
    {
        if (entry.second.IsEnabled())
        {
            ordered.push_back(ContextOrder{ entry.second.GetPriority(), entry.first });
        }
    }

    std::sort(
        ordered.begin(),
        ordered.end(),
        [](const ContextOrder& lhs, const ContextOrder& rhs)
        {
            return lhs.priority > rhs.priority;
        });

    std::vector<std::string> names;
    names.reserve(ordered.size());
    for (const ContextOrder& item : ordered)
    {
        names.push_back(item.name);
    }
    return names;
}

bool InputSystem::BindingModifiersMatch(const InputBinding& binding, const RawInputState& input) const
{
    const bool ctrlDown = input.Key(InputKey::Ctrl).down;
    const bool shiftDown = input.Key(InputKey::Shift).down;
    const bool altDown = input.Key(InputKey::Alt).down;

    if (binding.requireCtrl && !ctrlDown)
    {
        return false;
    }
    if (binding.requireShift && !shiftDown)
    {
        return false;
    }
    if (binding.requireAlt && !altDown)
    {
        return false;
    }
    return true;
}
