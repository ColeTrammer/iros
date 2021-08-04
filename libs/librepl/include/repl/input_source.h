#pragma once

#include <liim/string.h>
#include <repl/forward.h>

namespace Repl {
enum class InputResult {
    Eof,
    Error,
    Success,
};

class InputSource {
public:
    InputSource(ReplBase& repl);
    virtual ~InputSource();

    virtual InputResult get_input() = 0;

    const String& input_text() const { return m_input; }

    ReplBase& repl() { return m_repl; }

protected:
    void set_input(String input) { m_input = move(input); }
    void clear_input() { m_input.clear(); }

private:
    String m_input;
    ReplBase& m_repl;
};
}
