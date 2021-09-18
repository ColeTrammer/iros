#pragma once

#include <eventloop/event.h>
#include <eventloop/selectable.h>
#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/string.h>

APP_EVENT(App, PathChangeEvent, Event, (), ((String, path)), ())
APP_EVENT(App, PathRemovedEvent, Event, (), ((String, path)), ())

namespace App {
class FileWatcher final : public Selectable {
    APP_OBJECT(FileWatcher);

    APP_EMITS(Selectable, PathChangeEvent, PathRemovedEvent)

public:
    virtual void initialize() override;
    virtual ~FileWatcher() override;

    bool watch(const String& path);
    bool unwatch(const String& path);

private:
    FileWatcher();

    HashMap<int, String> m_identifier_to_path;
    HashMap<String, int> m_path_to_indentifier;
    int m_identifier_index { 1 };
};
}
