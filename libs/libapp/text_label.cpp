#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void TextLabel::render() {
    Renderer renderer(*window()->pixels());

    renderer.set_color(Color(0, 0, 0));
    renderer.fill_rect(rect());

    renderer.set_color(Color(255, 255, 255));
    renderer.render_text(rect().x(), rect().y(), text(), font());

    Widget::render();
}

}
