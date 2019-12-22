#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <stddef.h>
#include <stdio.h>

class ShValue {
public:
    struct IoRedirect {
        enum class Type {
            InputFileName,
            InputFileDescriptor,
            OutputFileName,
            OutputFileDescriptor,
            OutputFileNameAppend,
            InputAndOutputFileName,
            OutputFileNameClobber,
        };

        int number;
        Type type;
        StringView rhs;
    };

    using AssignmentWord = StringView;

    struct SimpleCommand {
        Vector<StringView> words;
        Vector<AssignmentWord> assignment_words;
        Vector<IoRedirect> redirect_info;
    };

    struct Command {
        enum class Type {
            Simple,
            Compound,
            FunctionDefinition,
        };

        Command(const ShValue::SimpleCommand& _simple_command) : type(Type::Simple), simple_command(_simple_command) {}

        Type type;
        Maybe<ShValue::SimpleCommand> simple_command;
    };

    struct Pipeline {
        bool negated;
        Vector<Command> commands;
    };

    struct ListComponent {
        enum class Combinator {
            And,
            Or,
            End,
        };

        Vector<Pipeline> pipelines;
        Vector<Combinator> combinators;
    };

    struct List {
        enum class Combinator {
            Sequential,
            Asynchronous,
        };

        Vector<ListComponent> components;
        Vector<Combinator> combinators;
    };

    using Program = Vector<List>;

    ShValue() {}
    ShValue(const StringView& text, size_t line, size_t position) : m_line(line), m_position(position), m_text(text) {}
    ShValue(const ShValue& other)
        : m_line(other.line())
        , m_position(other.position())
        , m_io_redirect(other.m_io_redirect)
        , m_text(other.m_text)
        , m_command(other.m_command)
        , m_pipeline(other.m_pipeline)
        , m_list_component(other.m_list_component)
        , m_list(other.m_list)
        , m_separator_op(other.m_separator_op)
        , m_program(other.m_program) {}

    size_t line() const { return m_line; }
    size_t position() const { return m_position; }

    const StringView& text() const { return m_text.value(); }

    bool has_text() const { return m_text.has_value(); }

    IoRedirect& io_redirect() { return m_io_redirect.value(); }
    const IoRedirect& io_redirect() const { return m_io_redirect.value(); }

    bool has_io_redirect() const { return m_io_redirect.has_value(); }

    ShValue& create_io_redirect(int number, IoRedirect::Type type, const StringView& word) {
        m_io_redirect = { IoRedirect { number, type, word } };

        return *this;
    }

    Command& command() { return m_command.value(); }
    const Command& command() const { return m_command.value(); }

    bool has_command() const { return m_command.has_value(); }

    ShValue& create_simple_command() {
        m_command = { SimpleCommand { Vector<StringView>(), Vector<AssignmentWord>(), Vector<IoRedirect>() } };
        return *this;
    }

    ShValue& create_simple_command(const StringView& word) {
        Vector<StringView> words;
        words.add(word);

        m_command = { SimpleCommand { words, Vector<AssignmentWord>(), Vector<IoRedirect>() } };
        return *this;
    }

    enum class AssignmentWordTag { True };

    ShValue& create_simple_command(const AssignmentWord& assignment_word, AssignmentWordTag) {
        Vector<AssignmentWord> words;
        words.add(assignment_word);

        m_command = { SimpleCommand { Vector<StringView>(), words, Vector<IoRedirect>() } };
        return *this;
    }

    ShValue& create_simple_command(const IoRedirect& io_redirect) {
        Vector<IoRedirect> redirects;
        redirects.add(io_redirect);

        m_command = { SimpleCommand { Vector<StringView>(), Vector<AssignmentWord>(), redirects } };
        return *this;
    }

    Pipeline& pipeline() { return m_pipeline.value(); }
    const Pipeline& pipeline() const { return m_pipeline.value(); }

    bool has_pipeline() const { return m_pipeline.has_value(); }

    ShValue& create_pipeline(const Command& command) {
        m_pipeline = { Pipeline { false, Vector<Command>() } };
        pipeline().commands.add(command);
        return *this;
    }

    ListComponent& list_component() { return m_list_component.value(); }
    const ListComponent& list_component() const { return m_list_component.value(); }

    bool has_list_component() const { return m_list_component.has_value(); }

    ShValue& create_list_component(const Pipeline& pipeline) {
        m_list_component = { ListComponent { Vector<Pipeline>(), Vector<ListComponent::Combinator>() } };
        list_component().pipelines.add(pipeline);
        list_component().combinators.add(ListComponent::Combinator::End);
        return *this;
    }

    List& list() { return m_list.value(); }
    const List& list() const { return m_list.value(); }

    bool has_list() const { return m_list.has_value(); }

    ShValue& create_list(const ListComponent& list_component, List::Combinator combinator = List::Combinator::Sequential) {
        m_list = { List { Vector<ListComponent>(), Vector<List::Combinator>() } };
        list().components.add(list_component);
        list().combinators.add(combinator);
        return *this;
    }

    List::Combinator separator_op() const { return m_separator_op.value(); }
    bool has_separator_op() const { return m_separator_op.has_value(); }

    ShValue& create_separator_op(List::Combinator combinator) {
        m_separator_op = { combinator };
        return *this;
    }

    Program& program() { return m_program.value(); }
    const Program& program() const { return m_program.value(); }

    bool has_program() const { return m_program.has_value(); }

    ShValue& create_program() {
        m_program = { Program {} };
        return *this;
    }

    ShValue& create_program(const List& list) {
        m_program = { Program {} };
        program().add(list);
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
    size_t m_line { 0 };
    size_t m_position { 0 };
    Maybe<IoRedirect> m_io_redirect;
    Maybe<StringView> m_text;
    Maybe<Command> m_command;
    Maybe<Pipeline> m_pipeline;
    Maybe<ListComponent> m_list_component;
    Maybe<List> m_list;
    Maybe<List::Combinator> m_separator_op;
    Maybe<Program> m_program;
};