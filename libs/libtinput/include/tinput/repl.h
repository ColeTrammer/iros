#pragma once

#include <edit/document_type.h>
#include <liim/string.h>

namespace TInput {

enum class InputResult {
    Eof,
    Empty,
    Error,
    Success,
};

enum class InputStatus {
    Incomplete,
    Finished,
};

class Repl {
public:
    virtual ~Repl();

    InputResult get_input();
    const String& input_string() const { return m_input; }

    virtual InputStatus get_input_status(const String& input) const = 0;

    virtual DocumentType get_input_type() const { return DocumentType::Text; }
    virtual String get_main_prompt() const;
    virtual String get_secondary_prompt() const;
    virtual Vector<String> get_suggestions(const String& input, size_t cursor_index) const;

private:
    String m_input;
};

}
