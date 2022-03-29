#include <app/base/view.h>
#include <tui/view.h>

namespace TUI {
View::View() : ScrollComponent(static_cast<Panel&>(*this)) {}

void View::did_attach() {
    ScrollComponent::attach_to_base(base());
    Panel::did_attach();
}

void View::invalidate_all() {
    invalidate();
}
}
