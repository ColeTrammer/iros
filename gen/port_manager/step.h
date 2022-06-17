#pragma once

#include <ext/json.h>
#include <liim/container/new_vector.h>
#include <liim/result.h>
#include <liim/span.h>
#include <liim/string.h>
#include "error.h"
#include "process.h"

namespace PortManager {
class Step {
public:
    virtual ~Step() {}

    virtual StringView name() const;
    virtual Result<bool, Error> should_skip(Context&, const Port&) { return false; }
    virtual Result<void, Error> act(Context& context, const Port& port) = 0;
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

    virtual Result<bool, Error> should_skip(Context& context, const Port& port) override;
    virtual Result<void, Error> act(Context& context, const Port& port) override;

private:
    String m_url;
};

class TarDownloadStep : public DownloadStep {
private:
    enum Kind {
        Gz,
    };

    static String kind_to_string(Kind kind);
    static Result<Kind, Ext::StringError> kind_from_string(const String& string);

public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    explicit TarDownloadStep(String url, Kind kind, String signature_url, String sourece_directory_in_tarball);
    virtual ~TarDownloadStep() override;

    virtual Result<bool, Error> should_skip(Context& context, const Port& port) override;
    virtual Result<void, Error> act(Context& context, const Port& port) override;

private:
    Ext::Path download_destination(const Port& port) const;
    Ext::Path signature_download_destination(const Port& port) const;

    String m_url;
    Kind m_kind { Kind::Gz };
    String m_signature_url;
    String m_source_directory_in_tarball;
};

class PatchStep : public Step {
public:
    static Result<UniquePtr<PatchStep>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    explicit PatchStep(NewVector<String> patch_files);
    virtual ~PatchStep() override;

    virtual StringView name() const override { return "patch"sv; }
    virtual Result<bool, Error> should_skip(Context& context, const Port& port) override;
    virtual Result<void, Error> act(Context& context, const Port& port) override;
    virtual Span<const StringView> dependencies() const override;

private:
    Ext::Path patch_path(const Port& port, const String& patch_name) const;
    Ext::Path patch_marker_path(const Port& port, const String& patch_name) const;

    NewVector<String> m_patch_files;
};

class ConfigureStep : public Step {
public:
    virtual StringView name() const override { return "configure"sv; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeConfigureStep : public ConfigureStep {
public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeConfigureStep() override;

    virtual Result<bool, Error> should_skip(Context& context, const Port& port) override;
    virtual Result<void, Error> act(Context& context, const Port& port) override;
};

class AutoconfConfigureStep : public ConfigureStep {
private:
    using Setting = Variant<bool, String>;
    using Settings = HashMap<String, Setting>;

public:
    static Result<Enviornment, Error> parse_enviornment(const JsonReader& reader, const Ext::Json::Object& object);
    static Result<Settings, Error> parse_settings(const JsonReader& reader, const Ext::Json::Object& object);
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    AutoconfConfigureStep(Enviornment enviornment, Settings settings);
    virtual ~AutoconfConfigureStep() override;

    virtual Result<bool, Error> should_skip(Context& context, const Port& port) override;
    virtual Result<void, Error> act(Context& context, const Port& port) override;

private:
    Enviornment m_enviornment;
    Settings m_settings;
};

class BuildStep : public Step {
    virtual StringView name() const override { return "build"; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeBuildStep : public BuildStep {
public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeBuildStep() override;

    virtual Result<void, Error> act(Context& context, const Port& port) override;
};

class AutoconfBuildStep : public BuildStep {
public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~AutoconfBuildStep() override;

    virtual Result<void, Error> act(Context& context, const Port& port) override;
};

class InstallStep : public Step {
    virtual StringView name() const override { return "install"; }
    virtual Span<const StringView> dependencies() const override;
};

class CMakeInstallStep : public InstallStep {
public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~CMakeInstallStep() override;

    virtual Result<void, Error> act(Context& context, const Port& port) override;
};

class AutoconfInstallStep : public InstallStep {
public:
    static Result<UniquePtr<Step>, Error> try_create(const JsonReader& reader, const Ext::Json::Object& object);

    virtual ~AutoconfInstallStep() override;

    virtual Result<void, Error> act(Context& context, const Port& port) override;
};

class CleanStep : public Step {
public:
    static Result<UniquePtr<CleanStep>, Error> try_create();

    virtual ~CleanStep();

    virtual StringView name() const override { return "clean"; }
    virtual Result<void, Error> act(Context& context, const Port& port) override;
};
}
