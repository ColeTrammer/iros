#pragma once

#include <eventloop/selectable.h>
#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/string.h>

namespace App {
class FileWatcher final : public Selectable {
    APP_OBJECT(FileWatcher);

public:
    FileWatcher();
    virtual ~FileWatcher() override;

    bool watch(const String& path);
    bool unwatch(const String& path);

    Function<void(const String&)> on_change;

private:
    virtual void notify_readable();

    HashMap<int, String> m_identifier_to_path;
    HashMap<String, int> m_path_to_indentifier;
    int m_identifier_index { 1 };
};
}
