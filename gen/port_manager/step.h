#pragma once

#include <ext/json.h>
#include <liim/result.h>
#include <liim/span.h>
#include <liim/string.h>
#include "error.h"
#include "forward.h"

namespace PortManager {
class Step {
public:
    virtual ~Step() {}

    virtual StringView name() const;
    virtual Result<Monostate, Error> act(Context& context, const Port& port) = 0;
    virtual Span<const StringView> dependencies() const { return {}; }
};

class DownloadStep : public Step {
public:
    static Result<UniquePtr<DownloadStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual StringView name() const override { return "download"sv; }
};

class GitDownloadStep : public DownloadStep {
public:
    static Result<UniquePtr<GitDownloadStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    explicit GitDownloadStep(String url);
    virtual ~GitDownloadStep() override;

    virtual Result<Monostate, Error> act(Context& context, const Port& port) override;

private:
    String m_url;
};

class PatchStep : public Step {
public:
    static Result<UniquePtr<PatchStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    explicit PatchStep(Vector<String> patch_files);
    virtual ~PatchStep() override;

    virtual StringView name() const override { return "patch"sv; }
    virtual Result<Monostate, Error> act(Context& context, const Port& port) override;
    virtual Span<const StringView> dependencies() const override;

private:
    Vector<String> m_patch_files;
};
}
