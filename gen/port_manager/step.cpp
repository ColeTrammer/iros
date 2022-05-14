#include <liim/format.h>
#include <liim/function.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/try.h>

#include "context.h"
#include "error.h"
#include "json_reader.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<UniquePtr<DownloadStep>, Error> DownloadStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& type = TRY(reader.lookup<Ext::Json::String>(object, "type"));
    if (type.view() == "git"sv) {
        return GitDownloadStep::try_create(reader, object);
    }

    return Err(StringError(format("Unknown download type: `{}'", type)));
}

Result<UniquePtr<GitDownloadStep>, Error> GitDownloadStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& url = TRY(reader.lookup<Ext::Json::String>(object, "url"));
    return Ok(make_unique<GitDownloadStep>(move(url)));
}

GitDownloadStep::GitDownloadStep(String url) : m_url(move(url)) {}

GitDownloadStep::~GitDownloadStep() {}

Result<Monostate, Error> GitDownloadStep::act(Context& context, const Port& port) {
    return context.run_command(format("git clone --depth=1 {} {}", m_url, port.source_directory())).map_error([&](auto) {
        return StringError(format("git clone on url `{}' failed", m_url));
    });
}

Result<UniquePtr<PatchStep>, Error> PatchStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& files = TRY(reader.lookup<Ext::Json::Array>(object, "files"));

    auto patch_files = Vector<String> {};
    for (auto& file : files) {
        if (!file.is<Ext::Json::String>()) {
            return Err(StringError("encountered non-string value when parsing patch files"));
        }
        patch_files.add(file.as<Ext::Json::String>());
    }

    return Ok(make_unique<PatchStep>(move(patch_files)));
}

PatchStep::PatchStep(Vector<String> patch_files) : m_patch_files(move(patch_files)) {}

PatchStep::~PatchStep() {}

Result<Monostate, Error> PatchStep::act(Context& context, const Port& port) {
    return context.with_working_directory(port.source_directory(), [&]() -> Result<Monostate, Error> {
        TRY(context.run_command("git init").map_error([&](auto) {
            return StringError("git init failed");
        }));

        for (auto& patch_file : m_patch_files) {
            TRY(context.run_command(format("git apply {}/{}", port.definition_directory(), patch_file)).map_error([&](auto) {
                return StringError(format("git patch failed with patch file `{}'", patch_file));
            }));
        }
        return Ok(Monostate {});
    });
}

Span<const StringView> PatchStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv };
    return storage.span();
}
}
