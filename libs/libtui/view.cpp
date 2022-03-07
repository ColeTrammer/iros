#include <app/base/view.h>
#include <tui/view.h>

namespace TUI {
void View::invalidate_all() {
    invalidate();
}
}
