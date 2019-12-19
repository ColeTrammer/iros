#pragma once

#include <liim/string.h>
#include <stdio.h>

#include "generic_sh_parser.h"
#include "sh_token.h"

class ShParser final : public GenericShParser<ShValue> {
public:
    using Token = GenericShParser<ShValue>::Token;

    ShParser(const Vector<Token>& tokens) : GenericShParser<ShValue>(tokens) {}
    virtual ~ShParser() override {}

    virtual ShValue reduce_cmd_suffix$cmd_suffix_word(ShValue& list, ShValue& new_value) override {
        if (new_value.text().has_value()) {
            list.words().add(new_value.text().value());
        }

        return list;
    }

    virtual ShValue reduce_linebreak() override { return { "Blank", 0, 0 }; }

    virtual ShValue reduce_cmd_suffix$word(ShValue& v) override { return v; }
    virtual ShValue reduce_cmd_name$word(ShValue& v) override { return v; }
    virtual ShValue reduce_simple_command$cmd_name(ShValue& v) override { return v; }
    virtual ShValue reduce_command$simple_command(ShValue& v) override { return v; }

    virtual ShValue reduce_simple_command$cmd_name_cmd_suffix(ShValue& list, ShValue& command_name) override {
        if (command_name.text().has_value()) {
            list.words().add(command_name.text().value());
        }

        fprintf(stderr, "Matched simple command.\n");
        list.words().for_each([&](const auto& s) {
            fprintf(stderr, "%s\n", String(s).string());
        });
        return list;
    }

    virtual ShValue reduce_pipe_sequence$command(ShValue& command) override {
        command.create_pipeline(command.words());
        return command;
    }

    virtual ShValue reduce_pipe_sequence$pipe_sequence_pipe_linebreak_command(ShValue& pipeline, ShValue&, ShValue&,
                                                                              ShValue& command) override {
        if (!pipeline.has_pipelines()) {
            pipeline.create_pipeline(pipeline.words());
        }
        pipeline.pipelines().add(command.words());
        return pipeline;
    }

    virtual ShValue reduce_pipeline$pipe_sequence(ShValue& pipe_sequence) override { return pipe_sequence; }
};