#include <app/layout.h>
#include <app/widget.h>

namespace App {

const Rect& Layout::container_rect() const {
    return widget().rect();
}

}
