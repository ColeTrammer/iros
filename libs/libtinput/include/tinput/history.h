#pragma once

#include <liim/string.h>
#include <liim/vector.h>

namespace TInput {

class History {
public:
    History() {}

    void add(String item) { m_history.add(move(item)); }
    void pop() { m_history.remove_last(); }

    int size() const { return m_history.size(); }
    const String& item(int index) const { return m_history[index]; }

private:
    Vector<String> m_history;
};

}
