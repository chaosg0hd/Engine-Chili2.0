#pragma once

#include "engine/script/script_event.hpp"
#include "prototypes/math/math.hpp"

#include <cstdint>
#include <functional>
#include <string_view>

namespace engine::script
{
    class ScriptContext
    {
    public:
        using LogSink = std::function<void(std::string_view)>;
        using EventSink = std::function<void(const ScriptEvent&)>;
        using WorldQueryFn = std::function<const TransformPrototype*(std::string_view)>;

        ScriptContext(
            std::uint64_t objectId,
            TransformPrototype& transform,
            LogSink logSink = {},
            EventSink eventSink = {});

        TransformPrototype& Transform();
        const TransformPrototype& Transform() const;
        std::uint64_t GetObjectId() const;
        void LogInfo(std::string_view message) const;
        void EmitEvent(const ScriptEvent& event) const;

        void SetWorldQuery(WorldQueryFn fn);
        const TransformPrototype* FindEntity(std::string_view name) const;

    private:
        std::uint64_t m_objectId = 0;
        TransformPrototype* m_transform = nullptr;
        LogSink m_logSink;
        EventSink m_eventSink;
        WorldQueryFn m_worldQuery;
    };
}
