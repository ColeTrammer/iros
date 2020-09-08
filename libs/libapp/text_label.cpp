#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void TextLabel::render() {
    Renderer renderer(*window()->pixels());

    renderer.fill_rect(rect(), ColorValue::Black);
    renderer.render_text(text(), rect(), ColorValue::White, text_align(), font());

    Widget::render();
}

}
