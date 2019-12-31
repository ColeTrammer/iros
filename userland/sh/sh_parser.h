#pragma once

#include <liim/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "generic_sh_parser.h"
#include "sh_lexer.h"
#include "sh_token.h"

class ShParser final : public GenericShParser<ShValue> {
public:
    using Token = GenericShParser<ShValue>::Token;

    ShParser(GenericLexer<ShTokenType, ShValue>& lexer) : GenericShParser<ShValue>(lexer) {}
    virtual ~ShParser() override {}

    virtual const ShValue& result() const { return this->peek_value_stack(); }

#if 1
    virtual ShValue reduce_program$linebreak(ShValue& l) override { return l.create_program(); }
    virtual ShValue reduce_program$linebreak_complete_commands_linebreak(ShValue&, ShValue& p, ShValue&) override {
        assert(p.has_program());
        return p;
    }

    virtual ShValue reduce_linebreak() override { return {}; }
    virtual ShValue reduce_linebreak$newline_list(ShValue&) override { return {}; }

    virtual ShValue reduce_newline_list$newline(ShValue&) override { return {}; }
    virtual ShValue reduce_newline_list$newline_list_newline(ShValue&, ShValue&) override { return {}; }

    virtual ShValue reduce_name$name(ShValue& n) override {
        assert(n.has_text());
        return n;
    }

    virtual ShValue reduce_filename$word(ShValue& word) override {
        assert(word.has_text());
        return word;
    }

    virtual ShValue reduce_io_file$lessthan_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDIN_FILENO, ShValue::IoRedirect::Type::InputFileName, word.text());
    }

    virtual ShValue reduce_io_file$lessand_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDIN_FILENO, ShValue::IoRedirect::Type::InputFileDescriptor, word.text());
    }

    virtual ShValue reduce_io_file$greaterthan_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileName, word.text());
    }

    virtual ShValue reduce_io_file$greatand_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileDescriptor, word.text());
    }

    virtual ShValue reduce_io_file$dgreat_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileNameAppend, word.text());
    }

    virtual ShValue reduce_io_file$lessgreat_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDIN_FILENO, ShValue::IoRedirect::Type::InputAndOutputFileName, word.text());
    }

    virtual ShValue reduce_io_file$clobber_filename(ShValue&, ShValue& word) {
        assert(word.has_text());
        return word.create_io_redirect(STDOUT_FILENO, ShValue::IoRedirect::Type::OutputFileNameClobber, word.text());
    }

    virtual ShValue reduce_io_redirect$io_file(ShValue& io_file) override {
        assert(io_file.has_io_redirect());
        return io_file;
    }

    virtual ShValue reduce_io_redirect$io_number_io_file(ShValue& io_number, ShValue& io_file) override {
        assert(io_file.has_io_redirect());
        assert(io_number.has_text());

        io_file.io_redirect().number = atoi(String(io_number.text()).string());
        return io_file;
    }

    virtual ShValue reduce_redirect_list$io_redirect(ShValue& io) override {
        assert(io.has_io_redirect());
        return io.create_redirect_list(io.io_redirect());
    }

    virtual ShValue reduce_redirect_list$redirect_list_io_redirect(ShValue& list, ShValue& io) override {
        assert(list.has_list());
        assert(io.has_io_redirect());

        list.redirect_list().add(io.io_redirect());
        return list;
    }

    virtual ShValue reduce_cmd_name$word(ShValue& v) override {
        assert(v.has_text());
        return v;
    }

    virtual ShValue reduce_cmd_word$word(ShValue& v) override {
        assert(v.has_text());
        return v;
    }

    virtual ShValue reduce_cmd_prefix$io_redirect(ShValue& io_redirect) override {
        assert(io_redirect.has_io_redirect());
        return io_redirect.create_simple_command(io_redirect.io_redirect());
    }

    virtual ShValue reduce_cmd_prefix$cmd_prefix_io_redirect(ShValue& sc, ShValue& io) override {
        assert(sc.has_command());
        assert(sc.command().type == ShValue::Command::Type::Simple);
        assert(io.has_io_redirect());

        sc.command().simple_command.value().redirect_info.add(io.io_redirect());
        return sc;
    }

    virtual ShValue reduce_cmd_prefix$assignment_word(ShValue& w) override {
        assert(w.has_text());
        return w.create_simple_command(ShValue::AssignmentWord { w.text() }, ShValue::AssignmentWordTag::True);
    }

    virtual ShValue reduce_cmd_prefix$cmd_prefix_assignment_word(ShValue& sc, ShValue& w) override {
        assert(sc.has_command());
        assert(sc.command().type == ShValue::Command::Type::Simple);
        assert(w.has_text());

        sc.command().simple_command.value().assignment_words.add(ShValue::AssignmentWord { w.text() });
        return sc;
    }

    virtual ShValue reduce_cmd_suffix$io_redirect(ShValue& io_redirect) override {
        assert(io_redirect.has_io_redirect());
        return io_redirect.create_simple_command(io_redirect.io_redirect());
    }

    virtual ShValue reduce_cmd_suffix$cmd_suffix_io_redirect(ShValue& simple_command, ShValue& io_redirect) override {
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);
        assert(io_redirect.has_io_redirect());

        simple_command.command().simple_command.value().redirect_info.add(io_redirect.io_redirect());
        return simple_command;
    }

    virtual ShValue reduce_cmd_suffix$word(ShValue& suffix_word) override {
        assert(suffix_word.has_text());
        return suffix_word.create_simple_command(suffix_word.text());
    }

    virtual ShValue reduce_cmd_suffix$cmd_suffix_word(ShValue& simple_command, ShValue& new_word) override {
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);
        assert(new_word.has_text());

        simple_command.command().simple_command.value().words.add(new_word.text());
        return simple_command;
    }

    virtual ShValue reduce_simple_command$cmd_name(ShValue& command_name) override {
        assert(command_name.has_text());
        return command_name.create_simple_command(command_name.text());
    }

    virtual ShValue reduce_simple_command$cmd_prefix_cmd_word_cmd_suffix(ShValue& simple_command, ShValue& name, ShValue& suffix) override {
        assert(name.has_text());
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);
        assert(suffix.has_command());
        assert(suffix.command().type == ShValue::Command::Type::Simple);

        simple_command.command().simple_command.value().words.add(name.text());

        // NOTE: the suffix can't have any assignment words
        auto& other = suffix.command().simple_command.value();
        other.words.for_each([&](const auto& s) {
            simple_command.command().simple_command.value().words.add(s);
        });
        other.redirect_info.for_each([&](const auto& i) {
            simple_command.command().simple_command.value().redirect_info.add(i);
        });
        return simple_command;
    }

    virtual ShValue reduce_simple_command$cmd_prefix_cmd_word(ShValue& simple_command, ShValue& name) {
        assert(name.has_text());
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);

        simple_command.command().simple_command.value().words.add(name.text());
        return simple_command;
    }

    virtual ShValue reduce_simple_command$cmd_prefix_cmd_name(ShValue& simple_command, ShValue& name) {
        assert(name.has_text());
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);

        simple_command.command().simple_command.value().words.add(name.text());
        return simple_command;
    }

    virtual ShValue reduce_simple_command$cmd_name_cmd_suffix(ShValue& command_name, ShValue& simple_command) override {
        assert(command_name.has_text());
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);

        simple_command.command().simple_command.value().words.insert(command_name.text(), 0);
        return simple_command;
    }

    virtual ShValue reduce_simple_command$cmd_prefix(ShValue& prefix) override {
        assert(prefix.has_command());
        assert(prefix.command().type == ShValue::Command::Type::Simple);
        return prefix;
    }

    virtual ShValue reduce_command$simple_command(ShValue& simple_command) override {
        assert(simple_command.has_command());
        assert(simple_command.command().type == ShValue::Command::Type::Simple);
        return simple_command;
    }

    virtual ShValue reduce_subshell$leftparenthesis_compound_list_rightparenthesis(ShValue&, ShValue& list, ShValue&) override {
        assert(list.has_list());
        return list.create_subshell(list.list());
    }

    virtual ShValue reduce_brace_group$lbrace_compound_list_rbrace(ShValue&, ShValue& list, ShValue&) override {
        assert(list.has_list());
        return list.create_brace_group(list.list());
    }

    virtual ShValue reduce_else_part$elif_compound_list_then_compound_list(ShValue&, ShValue& condition, ShValue&,
                                                                           ShValue& action) override {
        assert(condition.has_list());
        assert(action.has_list());

        return condition.create_if_clause({ condition.list() }, ShValue::IfClause::Condition::Type::Elif, action.list());
    }

    virtual ShValue reduce_else_part$elif_compound_list_then_compound_list_else_part(ShValue&, ShValue& condition, ShValue&,
                                                                                     ShValue& action, ShValue& if_clause) override {
        assert(if_clause.has_command());
        assert(if_clause.command().type == ShValue::Command::Type::Compound);
        assert(if_clause.command().compound_command.value().type == ShValue::CompoundCommand::Type::If);
        assert(condition.has_list());
        assert(action.has_list());

        ShValue::IfClause::Condition if_part = { condition.list(), ShValue::IfClause::Condition::Type::Elif, action.list() };
        if_clause.command().compound_command.value().if_clause.value().conditions.insert(if_part, 0);
        return if_clause;
    }

    virtual ShValue reduce_else_part$else_compound_list(ShValue&, ShValue& action) override {
        assert(action.has_list());
        return action.create_if_clause({}, ShValue::IfClause::Condition::Type::Else, action.list());
    }

    virtual ShValue reduce_if_clause$if_compound_list_then_compound_list_else_part_fi(ShValue&, ShValue& condition, ShValue&,
                                                                                      ShValue& action, ShValue& if_clause,
                                                                                      ShValue&) override {
        assert(if_clause.has_command());
        assert(if_clause.command().type == ShValue::Command::Type::Compound);
        assert(if_clause.command().compound_command.value().type == ShValue::CompoundCommand::Type::If);
        assert(condition.has_list());
        assert(action.has_list());

        ShValue::IfClause::Condition if_part = { condition.list(), ShValue::IfClause::Condition::Type::If, action.list() };
        if_clause.command().compound_command.value().if_clause.value().conditions.insert(if_part, 0);
        return if_clause;
    }

    virtual ShValue reduce_if_clause$if_compound_list_then_compound_list_fi(ShValue&, ShValue& condition, ShValue&, ShValue& action,
                                                                            ShValue&) override {
        assert(condition.has_list());
        assert(action.has_list());

        return condition.create_if_clause({ condition.list() }, ShValue::IfClause::Condition::Type::If, action.list());
    }

    virtual ShValue reduce_wordlist$wordlist_word(ShValue& if_clause, ShValue& word) override {
        assert(if_clause.has_command());
        assert(if_clause.command().type == ShValue::Command::Type::Compound);
        assert(if_clause.command().compound_command.value().type == ShValue::CompoundCommand::Type::For);
        assert(word.has_text());

        if_clause.command().compound_command.value().for_clause.value().words.add(word.text());
        return if_clause;
    }

    virtual ShValue reduce_wordlist$word(ShValue& word) {
        assert(word.has_text());
        Vector<StringView> words;
        words.add(word.text());

        return word.create_for_clause("__wordlist__", words,
                                      ShValue::List { Vector<ShValue::ListComponent>(), Vector<ShValue::List::Combinator>() });
    }

    virtual ShValue reduce_for_clause$for_name_do_group(ShValue&, ShValue& name, ShValue& list) override {
        assert(name.has_text());
        assert(list.has_list());

        return name.create_for_clause(name.text(), Vector<StringView>(), list.list());
    }

    virtual ShValue reduce_for_clause$for_name_sequential_sep_do_group(ShValue&, ShValue& name, ShValue&, ShValue& list) override {
        assert(name.has_text());
        assert(list.has_list());

        return name.create_for_clause(name.text(), Vector<StringView>(), list.list());
    }

    virtual ShValue reduce_for_clause$for_name_linebreak_in_sequential_sep_do_group(ShValue&, ShValue& name, ShValue&, ShValue&, ShValue&,
                                                                                    ShValue& list) override {
        assert(name.has_text());
        assert(list.has_list());

        return name.create_for_clause(name.text(), Vector<StringView>(), list.list());
    }

    virtual ShValue reduce_for_clause$for_name_linebreak_in_wordlist_sequential_sep_do_group(ShValue&, ShValue& name, ShValue&, ShValue&,
                                                                                             ShValue& words, ShValue&,
                                                                                             ShValue& list) override {
        assert(name.has_text());
        assert(list.has_list());
        assert(words.has_command());
        assert(words.command().type == ShValue::Command::Type::Compound);
        assert(words.command().compound_command.value().type == ShValue::CompoundCommand::Type::For);

        return name.create_for_clause(name.text(), words.command().compound_command.value().for_clause.value().words, list.list());
    }

    virtual ShValue reduce_compound_command$brace_group(ShValue& brace_group) override {
        assert(brace_group.has_command());
        assert(brace_group.command().type == ShValue::Command::Type::Compound);
        assert(brace_group.command().compound_command.value().type == ShValue::CompoundCommand::Type::BraceGroup);
        return brace_group;
    }

    virtual ShValue reduce_compound_command$subshell(ShValue& subshell) override {
        assert(subshell.has_command());
        assert(subshell.command().type == ShValue::Command::Type::Compound);
        assert(subshell.command().compound_command.value().type == ShValue::CompoundCommand::Type::Subshell);
        return subshell;
    }

    virtual ShValue reduce_compound_command$if_clause(ShValue& if_clause) override {
        assert(if_clause.has_command());
        assert(if_clause.command().type == ShValue::Command::Type::Compound);
        assert(if_clause.command().compound_command.value().type == ShValue::CompoundCommand::Type::If);
        return if_clause;
    }

    virtual ShValue reduce_compound_command$for_clause(ShValue& for_clause) override {
        assert(for_clause.has_command());
        assert(for_clause.command().type == ShValue::Command::Type::Compound);
        assert(for_clause.command().compound_command.value().type == ShValue::CompoundCommand::Type::For);
        return for_clause;
    }

    virtual ShValue reduce_command$compound_command(ShValue& compound_command) override {
        assert(compound_command.has_command());
        assert(compound_command.command().type == ShValue::Command::Type::Compound);
        return compound_command;
    }

    virtual ShValue reduce_command$compound_command_redirect_list(ShValue& compound_command, ShValue& list) override {
        assert(compound_command.has_command());
        assert(compound_command.command().type == ShValue::Command::Type::Compound);
        assert(list.has_redirect_list());

        compound_command.command().compound_command.value().redirect_list = list.redirect_list();
        return compound_command;
    }

    virtual ShValue reduce_pipe_sequence$command(ShValue& command) override {
        assert(command.has_command());
        return command.create_pipeline(command.command());
    }

    virtual ShValue reduce_pipe_sequence$pipe_sequence_pipe_linebreak_command(ShValue& pipeline, ShValue&, ShValue&,
                                                                              ShValue& command) override {
        assert(pipeline.has_pipeline());
        assert(command.has_command());
        pipeline.pipeline().commands.add(command.command());
        return pipeline;
    }

    virtual ShValue reduce_pipeline$pipe_sequence(ShValue& pipe_sequence) override {
        assert(pipe_sequence.has_pipeline());
        return pipe_sequence;
    }

    virtual ShValue reduce_pipeline$bang_pipe_sequence(ShValue&, ShValue& pipe_sequence) override {
        assert(pipe_sequence.has_pipeline());
        pipe_sequence.pipeline().negated = true;
        return pipe_sequence;
    }

    virtual ShValue reduce_and_or$pipeline(ShValue& pipeline) override {
        assert(pipeline.has_pipeline());
        return pipeline.create_list_component(pipeline.pipeline());
    }

    virtual ShValue reduce_and_or$and_or_and_if_linebreak_pipeline(ShValue& component, ShValue&, ShValue&, ShValue& pipeline) override {
        assert(component.has_list_component());
        assert(pipeline.has_pipeline());

        component.list_component().pipelines.add(pipeline.pipeline());
        component.list_component().combinators.last() = ShValue::ListComponent::Combinator::And;
        component.list_component().combinators.add(ShValue::ListComponent::Combinator::End);

        return component;
    }

    virtual ShValue reduce_and_or$and_or_or_if_linebreak_pipeline(ShValue& component, ShValue&, ShValue&, ShValue& pipeline) override {
        assert(component.has_list_component());
        assert(pipeline.has_pipeline());

        component.list_component().pipelines.add(pipeline.pipeline());
        component.list_component().combinators.last() = ShValue::ListComponent::Combinator::Or;
        component.list_component().combinators.add(ShValue::ListComponent::Combinator::End);
        return component;
    }

    virtual ShValue reduce_separator_op$semicolon(ShValue& semicolon) override {
        return semicolon.create_separator_op(ShValue::List::Combinator::Sequential);
    }

    virtual ShValue reduce_separator_op$ampersand(ShValue& ampersand) override {
        return ampersand.create_separator_op(ShValue::List::Combinator::Asynchronous);
    }

    virtual ShValue reduce_separator$newline_list(ShValue& newline_list) override {
        return newline_list.create_separator_op(ShValue::List::Combinator::Sequential);
    }

    virtual ShValue reduce_separator$separator_op_linebreak(ShValue& sep, ShValue&) override {
        assert(sep.has_separator_op());
        return sep;
    }

    virtual ShValue reduce_term$and_or(ShValue& list_component) override {
        assert(list_component.has_list_component());
        return list_component.create_list(list_component.list_component());
    }

    virtual ShValue reduce_term$term_separator_and_or(ShValue& list, ShValue& separator_op, ShValue& list_component) override {
        assert(list.has_list());
        assert(separator_op.has_separator_op());
        assert(list_component.has_list_component());

        list.list().components.add(list_component.list_component());
        list.list().combinators.last() = separator_op.separator_op();
        list.list().combinators.add(ShValue::List::Combinator::Sequential);
        return list;
    }

    virtual ShValue reduce_list$and_or(ShValue& list_component) override {
        assert(list_component.has_list_component());
        return list_component.create_list(list_component.list_component());
    }

    virtual ShValue reduce_list$list_separator_op_and_or(ShValue& list, ShValue& separator_op, ShValue& list_component) override {
        assert(list.has_list());
        assert(separator_op.has_separator_op());
        assert(list_component.has_list_component());

        list.list().components.add(list_component.list_component());
        list.list().combinators.last() = separator_op.separator_op();
        list.list().combinators.add(ShValue::List::Combinator::Sequential);
        return list;
    }

    virtual ShValue reduce_compound_list$linebreak_term(ShValue&, ShValue& list) override {
        assert(list.has_list());
        return list;
    }

    virtual ShValue reduce_compound_list$linebreak_term_separator(ShValue&, ShValue& list, ShValue& separator_op) override {
        assert(list.has_list());
        assert(separator_op.has_separator_op());

        list.list().combinators.last() = separator_op.separator_op();
        return list;
    }

    virtual ShValue reduce_do_group$do_compound_list_done(ShValue&, ShValue& compound_list, ShValue&) override {
        assert(compound_list.has_list());
        return compound_list;
    }

    virtual ShValue reduce_complete_command$list(ShValue& list) override {
        assert(list.has_list());
        return list;
    }

    virtual ShValue reduce_complete_command$list_separator_op(ShValue& list, ShValue& separator_op) override {
        assert(list.has_list());
        assert(separator_op.has_separator_op());

        list.list().combinators.last() = separator_op.separator_op();
        return list;
    }

    virtual ShValue reduce_complete_commands$complete_command(ShValue& list) override {
        assert(list.has_list());
        return list.create_program(list.list());
    }

    virtual ShValue reduce_complete_commands$complete_commands_newline_list_complete_command(ShValue& program, ShValue&,
                                                                                             ShValue& list) override {
        assert(program.has_program());
        assert(list.has_list());

        program.program().add(list.list());
        return program;
    }
#endif
};