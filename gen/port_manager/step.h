#pragma once

#include <liim/result.h>
#include <liim/span.h>
#include <liim/string.h>
#include "forward.h"

namespace PortManager {
class Step {
public:
    virtual ~Step() {}

    virtual StringView name() const;
    virtual Result<Monostate, String> act(Context& context) = 0;
    virtual Span<StringView> dependencies() { return {}; }
};

class DownloadStep : public Step {
public:
    virtual StringView name() const override { return "step"sv; }
};

class GitDownloadStep : public Step {
public:
    virtual Result<Monostate, String> act(Context& context) override;

private:
    String m_url;
};
}
