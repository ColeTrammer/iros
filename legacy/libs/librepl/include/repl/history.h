#pragma once

#include <liim/string.h>
#include <liim/vector.h>

namespace Repl {
class History {
public:
    History(String path, int history_max);
    ~History();

    void add(String item);
    void pop() { m_history.remove_last(); }

    int max() const { return m_history_max; }
    int size() const { return m_history.size(); }
    const String& item(int index) const { return m_history[index]; }

    void print_history();
    void read_history();
    void write_history();

    bool should_write_history() const { return m_should_write_history; }
    void set_should_write_history(bool b) { m_should_write_history = b; }

private:
    Vector<String> m_history;
    int m_history_max;
    String m_path;
    bool m_should_write_history { true };
};
}
