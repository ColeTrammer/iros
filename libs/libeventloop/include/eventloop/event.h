#pragma once

#include <eventloop/event_gen.h>
#include <liim/string_view.h>

namespace App {
class Event {
    APP_EVENT_HEADER(App, Event)

public:
    explicit Event(StringView name) : m_name(name) {}
    virtual ~Event() {}

    StringView name() const { return m_name; }

private:
    StringView m_name;
};
}