#pragma once

#include <repl/input_source.h>

namespace Repl {
class StringInputSource final : public InputSource {
public:
    StringInputSource(ReplBase& repl, String string);
    virtual ~StringInputSource() override;

    virtual InputResult get_input() override;

private:
    Vector<StringView> m_line_vector;
    int m_line_index { 0 };
    String m_string;
};
}
