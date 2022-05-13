#include <liim/format.h>
#include <liim/result.h>
#include <liim/try.h>

#include "context.h"
#include "error.h"
#include "json_reader.h"
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

GitDownloadStep::~GitDownloadStep() {}

Result<Monostate, Error> GitDownloadStep::act(Context& context) {
    return context.run_command(format("git clone --depth=1 {}", m_url)).map_error([&](auto) {
        return StringError(format("git clone on url `{}' failed", m_url));
    });
}

GitDownloadStep::GitDownloadStep(String url) : m_url(move(url)) {}
}
