#include <ctype.h>
#include <tinput/terminal_glyph.h>

namespace TInput {
Glyphs convert_to_glyphs(const StringView& view) {
    auto glyphs = Glyphs {};
    for (size_t i = 0; i < view.size(); i++) {
        if (view[i] == '\033') {
            auto start = i;
            while (i < view.size() && !isalpha(view[i])) {
                i++;
            }
            if (i == view.size()) {
                break;
            }
            glyphs.add({ String(view.substring(start, i - start + 1)), 0 });
            continue;
        }
        glyphs.add({ String(view[i]), 1 });
    }
    return glyphs;
}
}
