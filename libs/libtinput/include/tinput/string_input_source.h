#pragma once

#include <tinput/input_source.h>

namespace TInput {
class StringInputSource final : public InputSource {
public:
    StringInputSource(Repl& repl, String string);
    virtual ~StringInputSource() override;

    virtual InputResult get_input() override;

private:
    Vector<StringView> m_line_vector;
    int m_line_index { 0 };
    String m_string;
};
}
