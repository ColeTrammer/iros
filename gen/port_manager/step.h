#pragma once

#include <ext/json.h>
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
    static Result<UniquePtr<DownloadStep>, String> try_create(const Ext::Json::Object& object);

    virtual StringView name() const override { return "step"sv; }
};

class GitDownloadStep : public DownloadStep {
public:
    static Result<UniquePtr<GitDownloadStep>, String> try_create(const Ext::Json::Object& object);

    explicit GitDownloadStep(String url);
    virtual ~GitDownloadStep() override;

    virtual Result<Monostate, String> act(Context& context) override;

private:
    String m_url;
};
}
