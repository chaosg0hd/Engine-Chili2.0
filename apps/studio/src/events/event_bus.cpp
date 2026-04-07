#include "event_bus.hpp"

#include <utility>

void EventBus::Subscribe(EventSink sink)
{
    m_sinks.push_back(std::move(sink));
}

void EventBus::Publish(const std::string& eventName, const std::string& message) const
{
    for (const EventSink& sink : m_sinks)
    {
        if (sink)
        {
            sink(eventName, message);
        }
    }
}
