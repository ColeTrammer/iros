#include <liim/format.h>
#include <liim/result.h>
#include <liim/try.h>

#include "context.h"
#include "step.h"

namespace PortManager {
Result<UniquePtr<DownloadStep>, String> DownloadStep::try_create(const Ext::Json::Object& object) {
    auto read_json_string = [&](const Ext::Json::Object& object, const String& name) {
        return object.get_as<Ext::Json::String>(name).unwrap_or_else([&] {
            return format("Failed to read string property `{}'", name);
        });
    };

    auto& type = TRY(read_json_string(object, "type"));
    if (type.view() == "git"sv) {
        return GitDownloadStep::try_create(object);
    }

    return Err(format("Unknown download type: `{}'", type));
}

Result<UniquePtr<GitDownloadStep>, String> GitDownloadStep::try_create(const Ext::Json::Object& object) {
    auto read_json_string = [&](const Ext::Json::Object& object, const String& name) {
        return object.get_as<Ext::Json::String>(name).unwrap_or_else([&] {
            return format("Failed to read string property `{}'", name);
        });
    };

    auto& url = TRY(read_json_string(object, "url"));
    return Ok(make_unique<GitDownloadStep>(move(url)));
}

GitDownloadStep::~GitDownloadStep() {}

Result<Monostate, String> GitDownloadStep::act(Context& context) {
    return context.run_command(format("git clone --depth=1 {}", m_url)).map_error([&](auto) {
        return format("git clone on url `{}' failed with status code {}", m_url);
    });
}

GitDownloadStep::GitDownloadStep(String url) : m_url(move(url)) {}
}
