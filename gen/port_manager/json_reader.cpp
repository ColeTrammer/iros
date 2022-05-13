#include <ext/path.h>
#include <liim/format.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/try.h>

#include "json_reader.h"

namespace PortManager {
Result<JsonReader, StringError> JsonReader::try_create(Ext::Path path) {
    auto json = TRY(Ext::Json::parse_file(path.to_string()).unwrap_or_else([&] {
        return StringError(format("Failed to load JSON file: `{}'", path));
    }));

    return Ok(JsonReader(move(json), move(path)));
}

JsonReader::JsonReader(Ext::Json::Object json, Ext::Path path) : m_json(move(json)), m_path(move(path)) {}

JsonReader::~JsonReader() {}
}
