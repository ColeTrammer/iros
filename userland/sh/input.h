#pragma once

#include <liim/hash_map.h>
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

    struct Dirent {
        String name;
        mode_t mode;
    };

    const Vector<Dirent>& ensure_directory_entries(const String& directory) const;
    Suggestions suggest_executable(const String& path, const StringView& current_path, size_t suggestions_offset) const;
    Suggestions suggest_path_for(const String& path, const StringView& current_path, size_t suggestions_offset,
                                 bool should_be_executable) const;

    mutable HashMap<String, Vector<Dirent>> m_cached_directories;
};

void __refreshcwd();
char* __getcwd();
