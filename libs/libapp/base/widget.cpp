#include <app/base/widget.h>
#include <app/base/window.h>
#include <app/layout_engine.h>
#include <eventloop/event.h>

namespace App::Base {
Widget::Widget() {}

void Widget::initialize() {
    on<KeyDownEvent>([this](const KeyDownEvent& event) {
        return m_key_bindings.handle_key_event(event);
    });

    on<ShowEvent, HideEvent, ThemeChangeEvent>([this](const auto& event) {
        forward_to_children(event);
    });

    Object::initialize();
}

Widget::~Widget() {}

Maybe<Point> Widget::cursor_position() {
    return {};
}

void Widget::flush_layout() {
    for (auto& child : children()) {
        if (child->is_base_widget()) {
            auto& widget = const_cast<Widget&>(static_cast<const Widget&>(*child));
            widget.flush_layout();
        }
    }

    if (auto* engine = layout_engine()) {
        engine->maybe_force_layout();
    }
}

void Widget::render() {
    for (auto& child : children()) {
        if (child->is_base_widget()) {
            auto& widget = const_cast<Widget&>(static_cast<const Widget&>(*child));
            if (!widget.hidden()) {
                widget.render();
            }
        }
    }
}

void Widget::remove() {
    invalidate();

    if (auto* parent = parent_widget()) {
        if (auto* engine = parent->layout_engine()) {
            engine->remove(*this);
        }
    }

    if (parent()) {
        parent()->remove_child(shared_from_this());
    }
}

Widget* Widget::parent_widget() {
    if (!parent()) {
        return nullptr;
    }
    if (!parent()->is_base_widget()) {
        return nullptr;
    }
    return static_cast<Widget*>(parent());
}

Window* Widget::parent_window() {
    auto* object = parent();
    while (object) {
        if (object->is_window()) {
            return static_cast<Window*>(object);
        }
        object = object->parent();
    }
    return nullptr;
}

void Widget::invalidate() {
    invalidate(sized_rect());
}

void Widget::invalidate(const Rect& rect) {
    if (auto* window = parent_window()) {
        auto absolute_rect = rect.intersection_with(sized_rect()).translated(positioned_rect().top_left());
        if (!absolute_rect.empty()) {
            window->invalidate_rect(absolute_rect);
        }
    }
}

void Widget::make_focused() {
    auto& widget_to_check = focus_proxy() ? *focus_proxy() : *this;
    if (!widget_to_check.accepts_focus()) {
        if (auto* parent = parent_widget()) {
            return parent->make_focused();
        }
        return;
    }

    if (auto* window = parent_window()) {
        window->set_focused_widget(&widget_to_check);
    }
}

void Widget::set_hidden(bool b) {
    if (m_hidden == b) {
        return;
    }

    m_hidden = b;
    invalidate(positioned_rect());
    relayout();

    if (m_hidden) {
        emit<HideEvent>();
    } else {
        emit<ShowEvent>();
    }
}

void Widget::set_positioned_rect(const Rect& rect) {
    if (m_positioned_rect == rect) {
        return;
    }

    int old_width = m_positioned_rect.width();
    int old_height = m_positioned_rect.height();

    invalidate();
    m_positioned_rect = rect;
    invalidate();

    if (layout_engine()) {
        layout_engine()->schedule_layout();
    }

    if (old_width != rect.width() || old_height != rect.height()) {
        emit<ResizeEvent>();
    }
}

void Widget::set_layout_constraint(const LayoutConstraint& constraint) {
    if (m_layout_constraint.width() == constraint.width() && m_layout_constraint.height() == constraint.height()) {
        return;
    }

    m_layout_constraint = constraint;
    relayout();
}

void Widget::do_set_layout_engine(UniquePtr<LayoutEngine> engine) {
    m_layout_engine = move(engine);
}

void Widget::relayout() {
    if (auto* parent = parent_widget()) {
        if (auto* layout_engine = parent->layout_engine()) {
            layout_engine->schedule_layout();
        }
    }
}
}
