#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void TextLabel::render() {
    Renderer renderer(*window()->pixels());

    renderer.fill_rect(rect(), Color(0, 0, 0));
    renderer.render_text(rect().x(), rect().y(), text(), Color(255, 255, 255), font());

    Widget::render();
}

}
