#pragma once

#include <di/format/builtin_formatter/prelude.h>
#include <di/format/concepts/formattable.h>
#include <di/format/format_parse_context.h>
#include <di/format/present.h>
#include <di/format/present_encoded_context.h>
#include <di/format/style.h>
#include <di/format/to_string.h>

namespace di {
using concepts::FormatContext;
using concepts::Formattable;

using format::FormatParseContext;
using format::FormatStringImpl;
using format::Styled;

using FormatColor = format::Color;
using FormatBackgroundColor = format::BackgroundColor;
using FormatEffect = format::Effect;

using format::formatter_in_place;
using format::present;
using format::present_encoded_context;
using format::to_string;
}
