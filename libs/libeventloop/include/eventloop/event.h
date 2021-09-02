#pragma once

#include <eventloop/event_gen.h>

namespace App {
class Event {
    APP_EVENT_HEADER(App, Event)

public:
    struct FieldString {
        StringView name;
        String value;
    };

    explicit Event(StringView name) : m_name(name) {}
    virtual ~Event() {}

    StringView name() const { return m_name; }

    virtual Vector<FieldString> field_strings() const {
        return Vector<FieldString>::create_from_single_element(FieldString { "name"sv, String { m_name } });
    }

private:
    StringView m_name;
};
}

namespace LIIM::Format {
template<typename Ev>
requires(LIIM::DerivedFrom<Ev, App::Event>) struct Formatter<Ev> : public Formatter<String> {
    void format(const Ev& event, FormatContext& context) {
        auto fields = event.field_strings();
        auto vector = Vector<String> {};
        for (auto& field : fields) {
            vector.add(::format("{}={}", field.name, field.value));
        }
        Formatter<String>::format(::format("Event <{}>", String::join(vector, ',')), context);
    }
};
}
