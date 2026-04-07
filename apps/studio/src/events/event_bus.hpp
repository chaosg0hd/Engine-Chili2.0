#pragma once

#include <functional>
#include <string>
#include <vector>

class EventBus
{
public:
    using EventSink = std::function<void(const std::string& eventName, const std::string& message)>;

    void Subscribe(EventSink sink);
    void Publish(const std::string& eventName, const std::string& message) const;

private:
    std::vector<EventSink> m_sinks;
};
