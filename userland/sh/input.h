#pragma once

#include <liim/pointers.h>
#include <sh/sh_token.h>
#include <stdio.h>
#include <tinput/repl.h>

class ShRepl final : public TInput::Repl {
public:
    static ShRepl& the();

    ShRepl();
    virtual ~ShRepl() override;

private:
    virtual void did_get_input(const String& input) override;
    virtual void did_begin_loop_iteration() override;
    virtual void did_end_input() override;
    virtual bool force_stop_input() const override;

    virtual TInput::InputStatus get_input_status(const String& input) const override;
    virtual DocumentType get_input_type() const override { return DocumentType::ShellScript; }
    virtual String get_main_prompt() const override;
    virtual String get_secondary_prompt() const override { return "> "; }
    virtual Suggestions get_suggestions(const String& input, size_t position) const override;
};

void __refreshcwd();
char* __getcwd();
