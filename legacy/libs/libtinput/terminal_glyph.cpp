#include <ctype.h>
#include <liim/format.h>
#include <liim/utf8_view.h>
#include <tinput/terminal_glyph.h>
#include <unicode/terminal_width.h>

namespace TInput {
static bool is_unicode_mark(uint32_t code_point) {
    // FIXME: use unicode UCD instead.
    return (code_point >= 0x0300 && code_point <= 0x036F) || (code_point >= 0x1AB0 && code_point <= 0x1AFF) ||
           (code_point >= 0x1DC0 && code_point <= 0x1DFF) || (code_point >= 0x20D0 && code_point <= 0x20FF) ||
           (code_point >= 0xFE20 && code_point <= 0xFE2F);
}

Glyphs convert_to_glyphs(const StringView& view) {
    auto glyphs = Glyphs {};
    auto utf8 = Utf8View { view };
    for (auto it = utf8.begin(); it != utf8.end(); ++it) {
        auto info = it.current_code_point_info();
        if (info.codepoint && !glyphs.empty() && is_unicode_mark(*info.codepoint)) {
            // FIXME: combining marks probably should only combine under certain circumstances.
            glyphs.last().text().insert(view.substring(it.byte_offset(), info.bytes_used), glyphs.last().text().size());
            continue;
        }

        if (info.codepoint == 0x1BU) {
            auto start = it.byte_offset();
            while (it != utf8.end() && !isalpha(*it)) {
                ++it;
            }
            if (it == utf8.end()) {
                break;
            }
            glyphs.add({ String { view.substring(start, it.byte_offset() - start + 1) }, 0 });
            continue;
        }
        glyphs.add({ String { view.substring(it.byte_offset(), info.bytes_used) }, Unicode::terminal_code_point_width(*it) });
    }
    return glyphs;
}
}
