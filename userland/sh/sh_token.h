#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <stddef.h>
#include <stdio.h>

class ShValue {
public:
    struct Token {
        size_t line;
        size_t position;
        StringView text;
    };

    struct IoRedirect {
        enum class HereDocumentType {
            Regular,
            RemoveLeadingTabs,
        };

        enum class HereDocumentQuoted {
            Yes,
            No,
        };

        enum class Type {
            InputFileName,
            InputFileDescriptor,
            OutputFileName,
            OutputFileDescriptor,
            OutputFileNameAppend,
            InputAndOutputFileName,
            OutputFileNameClobber,
            HereDocument,
            HereString,
        };

        int number;
        Type type;
        StringView rhs;
        HereDocumentType here_document_type { HereDocumentType::Regular };
        HereDocumentQuoted here_document_quoted { HereDocumentQuoted::No };
    };

    using AssignmentWord = StringView;
    using RedirectList = Vector<IoRedirect>;

    struct SimpleCommand {
        Vector<StringView> words;
        Vector<AssignmentWord> assignment_words;
        Vector<IoRedirect> redirect_info;
    };

    struct Command;

    struct Pipeline {
        Pipeline(bool _negated, Vector<Command>&& _commands) : negated(_negated), commands(move(_commands)) {}

        bool negated;
        Vector<Command> commands;
    };

    struct ListComponent {
        enum class Combinator {
            And,
            Or,
            End,
        };

        ListComponent(Vector<Pipeline>&& _pipelines, Vector<Combinator>&& _combinators)
            : pipelines(move(_pipelines)), combinators(move(_combinators)) {}

        Vector<Pipeline> pipelines;
        Vector<Combinator> combinators;
    };

    struct List {
        enum class Combinator {
            Sequential,
            Asynchronous,
        };

        List() = default;

        List(Vector<ListComponent>&& _components, Vector<Combinator>&& _combinators)
            : components(move(_components)), combinators(move(_combinators)) {}

        Vector<ListComponent> components;
        Vector<Combinator> combinators;
    };

    struct ForClause {
        StringView name;
        Vector<StringView> words;
        ShValue::List action;
    };

    struct IfClause {
        struct Condition {
            enum class Type {
                If,
                Elif,
                Else,
            };

            Maybe<ShValue::List> condition;
            Type type;
            ShValue::List action;
        };

        Vector<Condition> conditions;
    };

    using BraceGroup = List;
    using Subshell = List;

    struct Loop {
        enum class Type {
            While,
            Until,
        };

        Type type;
        List condition;
        List action;
    };

    struct CaseClause {
        struct CaseItem {
            Vector<StringView> patterns;
            List action;
        };

        StringView word;
        Vector<CaseItem> items;
    };

    struct CompoundCommand {
        enum class Type {
            BraceGroup,
            Subshell,
            For,
            Case,
            If,
            Loop,
        };

        enum class MakeBraceGroup { Yes };
        enum class MakeSubshell { Yes };

        CompoundCommand(const IfClause& _if_clause) : type(Type::If), if_clause(_if_clause) {}
        CompoundCommand(const ForClause& _for_clause) : type(Type::For), for_clause(_for_clause) {}
        CompoundCommand(const BraceGroup& _brace_group, MakeBraceGroup) : type(Type::BraceGroup), brace_group(_brace_group) {}
        CompoundCommand(const Subshell& _subshell, MakeSubshell) : type(Type::Subshell), subshell(_subshell) {}
        CompoundCommand(const Loop& _loop) : type(Type::Loop), loop(_loop) {}
        CompoundCommand(const CaseClause& _case_clause) : type(Type::Case), case_clause(_case_clause) {}

        Type type;
        Maybe<ShValue::IfClause> if_clause;
        Maybe<ShValue::ForClause> for_clause;
        Maybe<ShValue::BraceGroup> brace_group;
        Maybe<ShValue::Subshell> subshell;
        Maybe<ShValue::Loop> loop;
        Maybe<ShValue::CaseClause> case_clause;
        RedirectList redirect_list;
    };

    struct FunctionDefinition {
        StringView name;
        CompoundCommand command;
    };

    struct Command {
        enum class Type {
            Simple,
            Compound,
            FunctionDefinition,
        };

        Command(const ShValue::SimpleCommand& _simple_command) : type(Type::Simple), simple_command(_simple_command) {}
        Command(const ShValue::CompoundCommand& _compound_command) : type(Type::Compound), compound_command(_compound_command) {}
        Command(const ShValue::FunctionDefinition& _function_definition)
            : type(Type::FunctionDefinition), function_definition(_function_definition) {}

        Type type;
        Maybe<ShValue::SimpleCommand> simple_command;
        Maybe<ShValue::CompoundCommand> compound_command;
        Maybe<ShValue::FunctionDefinition> function_definition;
    };

    using Program = Vector<List>;

    ShValue() {}
    ShValue(const StringView& text, size_t line, size_t position) : m_variant(Token { line, position, text }) {}

    ~ShValue() {}

    size_t line() const { return m_variant.as<Token>().line; }
    size_t position() const { return m_variant.as<Token>().line; }

    const StringView& text() const { return m_variant.as<Token>().text; }

    bool has_text() const { return m_variant.is<Token>(); }

    IoRedirect& io_redirect() { return m_variant.as<IoRedirect>(); }
    const IoRedirect& io_redirect() const { return m_variant.as<IoRedirect>(); }

    bool has_io_redirect() const { return m_variant.is<IoRedirect>(); }

    ShValue& create_io_redirect(int number, IoRedirect::Type type, const StringView& word,
                                ShValue::IoRedirect::HereDocumentQuoted quoted = ShValue::IoRedirect::HereDocumentQuoted::No) {
        m_variant = IoRedirect { number, type, word, ShValue::IoRedirect::HereDocumentType::Regular, quoted };

        return *this;
    }

    CaseClause::CaseItem& case_item() { return m_variant.as<CaseClause::CaseItem>(); }
    const CaseClause::CaseItem& case_item() const { return m_variant.as<CaseClause::CaseItem>(); }

    bool has_case_item() const { return m_variant.is<CaseClause::CaseItem>(); }

    ShValue& create_case_item(const StringView& pattern) {
        Vector<StringView> patterns;
        patterns.add(pattern);

        m_variant = CaseClause::CaseItem { patterns, List() };
        return *this;
    }

    RedirectList& redirect_list() { return m_variant.as<RedirectList>(); }
    const RedirectList& redirect_list() const { return m_variant.as<RedirectList>(); }

    bool has_redirect_list() const { return m_variant.is<RedirectList>(); }

    ShValue& create_redirect_list(const IoRedirect& io) {
        RedirectList list;
        list.add(io);

        m_variant = list;
        return *this;
    }

    Command& command() { return m_variant.as<Command>(); }
    const Command& command() const { return m_variant.as<Command>(); }

    bool has_command() const { return m_variant.is<Command>(); }

    ShValue& create_simple_command() {
        m_variant = SimpleCommand { Vector<StringView>(), Vector<AssignmentWord>(), Vector<IoRedirect>() };
        return *this;
    }

    ShValue& create_simple_command(const StringView& word) {
        Vector<StringView> words;
        words.add(word);

        m_variant = SimpleCommand { words, Vector<AssignmentWord>(), Vector<IoRedirect>() };
        return *this;
    }

    enum class AssignmentWordTag { True };

    ShValue& create_simple_command(const AssignmentWord& assignment_word, AssignmentWordTag) {
        Vector<AssignmentWord> words;
        words.add(assignment_word);

        m_variant = SimpleCommand { Vector<StringView>(), words, Vector<IoRedirect>() };
        return *this;
    }

    ShValue& create_simple_command(const IoRedirect& io_redirect) {
        Vector<IoRedirect> redirects;
        redirects.add(io_redirect);

        m_variant = SimpleCommand { Vector<StringView>(), Vector<AssignmentWord>(), redirects };
        return *this;
    }

    ShValue& create_if_clause(Maybe<ShValue::List> condition, ShValue::IfClause::Condition::Type type, ShValue::List action) {
        IfClause::Condition part = IfClause::Condition { condition, type, action };
        IfClause if_clause;
        if_clause.conditions.add(part);

        m_variant = CompoundCommand { if_clause };
        return *this;
    }

    ShValue& create_for_clause(StringView name, const Vector<StringView>& words, const ShValue::List& action) {
        ForClause for_clause = ForClause { name, words, action };

        m_variant = CompoundCommand { for_clause };
        return *this;
    }

    ShValue& create_case_clause(const CaseClause::CaseItem& case_item) {
        Vector<CaseClause::CaseItem> case_items;
        case_items.add(case_item);
        CaseClause case_clause = CaseClause { "", case_items };

        m_variant = CompoundCommand { case_clause };
        return *this;
    }

    ShValue& create_case_clause(const StringView& word) {
        CaseClause case_clause = CaseClause { word, Vector<CaseClause::CaseItem>() };

        m_variant = CompoundCommand { case_clause };
        return *this;
    }

    ShValue& create_brace_group(const ShValue::List& list) {
        m_variant = CompoundCommand { list, CompoundCommand::MakeBraceGroup::Yes };
        return *this;
    }

    ShValue& create_subshell(const ShValue::List& list) {
        m_variant = CompoundCommand { list, CompoundCommand::MakeSubshell::Yes };
        return *this;
    }

    ShValue& create_loop(const ShValue::List& condition, const ShValue::List& action, Loop::Type type) {
        Loop loop = Loop { type, condition, action };

        m_variant = CompoundCommand { loop };
        return *this;
    }

    ShValue& create_function_definition(const ShValue::CompoundCommand& command) {
        m_variant = FunctionDefinition { "", command };
        return *this;
    }

    Pipeline& pipeline() { return m_variant.as<Pipeline>(); }
    const Pipeline& pipeline() const { return m_variant.as<Pipeline>(); }

    bool has_pipeline() const { return m_variant.is<Pipeline>(); }

    ShValue& create_pipeline(const Command& command) {
        Vector<Command> commands;
        commands.add(move(command));
        m_variant.emplace<Pipeline>(false, move(commands));
        return *this;
    }

    ListComponent& list_component() { return m_variant.as<ListComponent>(); }
    const ListComponent& list_component() const { return m_variant.as<ListComponent>(); }

    bool has_list_component() const { return m_variant.is<ListComponent>(); }

    ShValue& create_list_component(const Pipeline& pipeline) {
        Vector<Pipeline> pipelines;
        pipelines.add(move(pipeline));
        Vector<ListComponent::Combinator> combinators;
        combinators.add(ListComponent::Combinator::End);

        m_variant.emplace<ListComponent>(move(pipelines), move(combinators));
        return *this;
    }

    List& list() { return m_variant.as<List>(); }
    const List& list() const { return m_variant.as<List>(); }

    bool has_list() const { return m_variant.is<List>(); }

    ShValue& create_list(const ListComponent& list_component, List::Combinator combinator = List::Combinator::Sequential) {
        Vector<ListComponent> components;
        Vector<List::Combinator> combinators;
        components.add(LIIM::move(list_component));
        combinators.add(combinator);

        m_variant.emplace<List>(move(components), move(combinators));
        return *this;
    }

    List::Combinator separator_op() const { return m_variant.as<List::Combinator>(); }
    bool has_separator_op() const { return m_variant.is<List::Combinator>(); }

    ShValue& create_separator_op(List::Combinator combinator) {
        m_variant = combinator;
        return *this;
    }

    Program& program() { return m_variant.as<Program>(); }
    const Program& program() const { return m_variant.as<Program>(); }

    bool has_program() const { return m_variant.is<Program>(); }

    ShValue& create_program() {
        m_variant = Program {};
        return *this;
    }

    ShValue& create_program(const List& list) {
        Vector<List> lists;
        lists.add(LIIM::move(list));
        m_variant.emplace<Program>(LIIM::move(lists));
        return *this;
    }

    static void dump(Program& program) {
        program.for_each([&](ShValue::List& list) {
            fprintf(stderr, " List\n");
            for (int i = 0; i < list.components.size(); i++) {
                ShValue::ListComponent& component = list.components[i];
                fprintf(stderr, "  Component\n");
                for (int i = 0; i < component.pipelines.size(); i++) {
                    ShValue::Pipeline& pipeline = component.pipelines[i];
                    fprintf(stderr, "   Pipeline%s\n", pipeline.negated ? " [ negated ]" : "");
                    pipeline.commands.for_each([&](ShValue::Command& command) {
                        switch (command.type) {
                            case ShValue::Command::Type::Simple: {
                                fprintf(stderr, "    Simple Command\n");
                                command.simple_command.value().assignment_words.for_each([&](auto& word) {
                                    fprintf(stderr, "     Eq: %s\n", String(word).string());
                                });
                                command.simple_command.value().redirect_info.for_each([&](ShValue::IoRedirect& io) {
                                    switch (io.type) {
                                        case ShValue::IoRedirect::Type::InputAndOutputFileName:
                                            fprintf(stderr, "     IO: %d <> %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::InputFileDescriptor:
                                            fprintf(stderr, "     IO: %d <& %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::InputFileName:
                                            fprintf(stderr, "     IO: %d < %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::OutputFileDescriptor:
                                            fprintf(stderr, "     IO: %d >& %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::OutputFileName:
                                            fprintf(stderr, "     IO: %d > %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::OutputFileNameAppend:
                                            fprintf(stderr, "     IO: %d >> %s\n", io.number, String(io.rhs).string());
                                            break;
                                        case ShValue::IoRedirect::Type::OutputFileNameClobber:
                                            fprintf(stderr, "     IO: %d >| %s\n", io.number, String(io.rhs).string());
                                            break;
                                        default:
                                            fprintf(stderr, "     IO: Invalid?\n");
                                    }
                                });
                                fprintf(stderr, "     ");
                                command.simple_command.value().words.for_each([&](auto& w) {
                                    fprintf(stderr, "%s ", String(w).string());
                                });
                                fprintf(stderr, "\n");
                                break;
                            }
                            default:
                                fprintf(stderr, "    Invalid command?\n");
                        }
                    });
                    fprintf(stderr, "%s\n",
                            component.combinators[i] == ShValue::ListComponent::Combinator::And
                                ? "   [ and ]"
                                : component.combinators[i] == ShValue::ListComponent::Combinator::Or ? "   [ or ]" : "   [ end ]");
                }

                fprintf(stderr, "%s\n",
                        list.combinators[i] == ShValue::List::Combinator::Asynchronous ? "  [ asynchronous ]" : "  [ synchronous ]");
            }
        });
    }

private:
    Variant<Monostate, Token, IoRedirect, CaseClause::CaseItem, RedirectList, Command, Pipeline, ListComponent, List, List::Combinator,
            Program>
        m_variant;
    // Maybe<Token> m_variant;
    // Maybe<IoRedirect> m_variant;
    // Maybe<CaseClause::CaseItem> m_variant;
    // Maybe<RedirectList> m_variant;
    // Maybe<Command> m_variant;
    // Maybe<Pipeline> m_variant;
    // Maybe<ListComponent> m_variant;
    // Maybe<List> m_variant;
    // Maybe<List::Combinator> m_variant;
    // Maybe<Program> m_variant;
};