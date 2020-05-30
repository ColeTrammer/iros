#include <app/text_label.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void TextLabel::render() {
    Renderer renderer(*window()->pixels());
    renderer.render_text(rect().x(), rect().y(), text());

    Widget::render();
}

}
