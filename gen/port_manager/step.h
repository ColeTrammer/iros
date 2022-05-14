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

class ConfigureStep : public Step {
public:
    virtual StringView name() const override { return "configure"sv; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeConfigureStep : public ConfigureStep {
public:
    static Result<UniquePtr<CMakeConfigureStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeConfigureStep() override;

    virtual Result<Monostate, Error> act(Context& context, const Port& port) override;
};

class BuildStep : public Step {
    virtual StringView name() const override { return "build"; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeBuildStep : public BuildStep {
public:
    static Result<UniquePtr<CMakeBuildStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeBuildStep() override;

    virtual Result<Monostate, Error> act(Context& context, const Port& port) override;
};

class InstallStep : public Step {
    virtual StringView name() const override { return "install"; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeInstallStep : public InstallStep {
public:
    static Result<UniquePtr<CMakeInstallStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeInstallStep() override;

    virtual Result<Monostate, Error> act(Context& context, const Port& port) override;
};
}
