#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void TextLabel::render() {
    Renderer renderer(*window()->pixels());

    renderer.fill_rect(positioned_rect(), background_color());
    renderer.render_text(text(), positioned_rect(), text_color(), text_align(), font());

    Widget::render();
}

}
