#include <liim/container/path.h>
#include <liim/format.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/try.h>

#include "json_reader.h"

namespace PortManager {
Result<JsonReader, Error> JsonReader::create(Path path) {
    auto json = TRY(Ext::Json::parse_file(path.clone().into_string()).unwrap_or_else([&] {
        return make_string_error("Failed to load JSON file: `{}'", path);
    }));

    return JsonReader(move(json), move(path));
}

JsonReader::JsonReader(Ext::Json::Object json, Path path) : m_json(move(json)), m_path(move(path)) {}

JsonReader::~JsonReader() {}
}
