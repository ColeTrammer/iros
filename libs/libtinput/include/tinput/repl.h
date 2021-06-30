#pragma once

#include <edit/document_type.h>
#include <liim/pointers.h>
#include <tinput/history.h>

class Suggestions;

namespace TInput {

class InputSource;
class History;

enum class InputStatus {
    Incomplete,
    Finished,
};

class Repl {
public:
    explicit Repl(UniquePtr<History> history);
    virtual ~Repl();

    History& history() { return *m_history; }
    const History& history() const { return *m_history; }

    void enter(InputSource& input_source);

    virtual InputStatus get_input_status(const String& input) const = 0;

    virtual Edit::DocumentType get_input_type() const { return Edit::DocumentType::Text; }
    virtual String get_main_prompt() const;
    virtual String get_secondary_prompt() const;
    virtual Edit::Suggestions get_suggestions(const String& input, size_t cursor_index) const;

protected:
    virtual void did_get_input(const String& input) = 0;

    virtual bool force_stop_input() const { return false; }
    virtual void did_begin_loop_iteration() {}
    virtual void did_begin_input() {}
    virtual void did_end_input() {}

private:
    UniquePtr<History> m_history;
};

}
