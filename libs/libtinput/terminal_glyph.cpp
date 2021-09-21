#include <ctype.h>
#include <liim/utf8_view.h>
#include <tinput/terminal_glyph.h>

namespace TInput {
Glyphs convert_to_glyphs(const StringView& view) {
    auto glyphs = Glyphs {};
    auto utf8 = Utf8View { view };
    for (auto it = utf8.begin(); it != utf8.end(); ++it) {
        auto info = it.current_code_point_info();
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
        glyphs.add({ String { view.substring(it.byte_offset(), info.bytes_used) }, 1 });
    }
    return glyphs;
}
}
