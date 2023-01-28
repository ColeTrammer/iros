#include <app/window.h>
#include <graphics/renderer.h>
#include <gui/text_label.h>

namespace GUI {
void TextLabel::render() {
    auto renderer = get_renderer();

    renderer.fill_rect(sized_rect(), background_color());
    renderer.render_text(text(), sized_rect(), text_color(), text_align(), *font());

    Widget::render();
}
}
