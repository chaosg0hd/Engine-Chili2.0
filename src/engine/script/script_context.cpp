#include "engine/script/script_context.hpp"

#include <utility>

namespace engine::script
{
    ScriptContext::ScriptContext(
        std::uint64_t objectId,
        TransformPrototype& transform,
        LogSink logSink,
        EventSink eventSink)
        : m_objectId(objectId)
        , m_transform(&transform)
        , m_logSink(std::move(logSink))
        , m_eventSink(std::move(eventSink))
    {
    }

    TransformPrototype& ScriptContext::Transform()
    {
        return *m_transform;
    }

    const TransformPrototype& ScriptContext::Transform() const
    {
        return *m_transform;
    }

    std::uint64_t ScriptContext::GetObjectId() const
    {
        return m_objectId;
    }

    void ScriptContext::LogInfo(std::string_view message) const
    {
        if (m_logSink)
        {
            m_logSink(message);
        }
    }

    void ScriptContext::EmitEvent(const ScriptEvent& event) const
    {
        if (m_eventSink)
        {
            m_eventSink(event);
        }
    }

    void ScriptContext::SetWorldQuery(WorldQueryFn fn)
    {
        m_worldQuery = std::move(fn);
    }

    const TransformPrototype* ScriptContext::FindEntity(std::string_view name) const
    {
        if (m_worldQuery)
        {
            return m_worldQuery(name);
        }
        return nullptr;
    }
}
