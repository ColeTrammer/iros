#pragma once

#include <repl/input_source.h>

namespace Repl {
class TerminalInputSource final : public InputSource {
public:
    TerminalInputSource(ReplBase& repl);
    virtual ~TerminalInputSource() override;

    virtual InputResult get_input() override;
};
}
