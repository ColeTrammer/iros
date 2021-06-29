#include <app/layout.h>
#include <app/widget.h>

namespace App {

SharedPtr<Widget> Layout::widget_pointer() {
    return m_widget.shared_from_this();
}

const Rect& Layout::container_rect() const {
    return widget().positioned_rect();
}

}
