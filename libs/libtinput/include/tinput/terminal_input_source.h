#pragma once

#include <tinput/input_source.h>

namespace TInput {
class TerminalInputSource final : public InputSource {
public:
    TerminalInputSource(Repl& repl);
    virtual ~TerminalInputSource() override;

    virtual InputResult get_input() override;
};
}
