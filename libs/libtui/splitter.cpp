#include <tinput/terminal_renderer.h>
#include <tui/splitter.h>

namespace TUI {
Splitter::Splitter() {}

Splitter::~Splitter() {}

void Splitter::render() {
    auto renderer = get_renderer();

    renderer.set_origin({ 0, 0 });

    for (auto& child : children()) {
        if (child->is_base_widget()) {
            auto& widget = static_cast<const App::Widget&>(*child);
            renderer.draw_rect(widget.positioned_rect().adjusted(1), { ColorValue::White });
        }
    }

    ParentWidget::render();
}
}
