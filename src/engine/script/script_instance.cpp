#include "engine/script/script_instance.hpp"

namespace engine::script
{
    ScriptInstance::ScriptInstance(std::unique_ptr<IScriptBehavior> behavior)
        : m_behavior(std::move(behavior))
    {
    }

    void ScriptInstance::SetBehavior(std::unique_ptr<IScriptBehavior> behavior)
    {
        m_behavior = std::move(behavior);
        m_constructed = false;
        m_started = false;
    }

    bool ScriptInstance::HasBehavior() const
    {
        return m_behavior != nullptr;
    }

    void ScriptInstance::Construct(ScriptContext& ctx)
    {
        if (!m_behavior || m_constructed)
        {
            return;
        }

        m_behavior->OnConstruct(ctx);
        m_constructed = true;
    }

    void ScriptInstance::Start(ScriptContext& ctx)
    {
        if (!m_behavior || !m_constructed || m_started)
        {
            return;
        }

        m_behavior->OnStart(ctx);
        m_started = true;
    }

    void ScriptInstance::Update(ScriptContext& ctx, float dt)
    {
        if (!m_behavior)
        {
            return;
        }

        if (!m_constructed)
        {
            Construct(ctx);
        }
        if (!m_started)
        {
            Start(ctx);
        }

        m_behavior->OnUpdate(ctx, dt);
    }

    void ScriptInstance::SendEvent(ScriptContext& ctx, const ScriptEvent& event)
    {
        if (!m_behavior)
        {
            return;
        }

        if (!m_constructed)
        {
            Construct(ctx);
        }
        if (!m_started)
        {
            Start(ctx);
        }

        m_behavior->OnEvent(ctx, event);
    }

    void ScriptInstance::Destruct(ScriptContext& ctx)
    {
        if (!m_behavior)
        {
            return;
        }

        if (m_constructed)
        {
            m_behavior->OnDestruct(ctx);
        }

        m_behavior.reset();
        m_constructed = false;
        m_started = false;
    }
}
