#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {
void TextLabel::render() {
    auto renderer = get_renderer();

    renderer.fill_rect(sized_rect(), background_color());
    renderer.render_text(text(), sized_rect(), text_color(), text_align(), *font());

    Widget::render();
}
}
