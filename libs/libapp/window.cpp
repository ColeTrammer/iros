#include <app/application.h>
#include <app/application_os_2.h>
#include <app/context_menu.h>
#include <app/widget.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <pthread.h>

namespace App {

static HashMap<wid_t, Window*> s_windows;
static pthread_mutex_t s_windows_lock = PTHREAD_MUTEX_INITIALIZER;

void Window::for_each_window(Function<void(Window&)> callback) {
    pthread_mutex_lock(&s_windows_lock);
    s_windows.for_each([&](auto* window) {
        callback(*window);
    });
    pthread_mutex_unlock(&s_windows_lock);
}

void Window::register_window(Window& window) {
    pthread_mutex_lock(&s_windows_lock);
    s_windows.put(window.wid(), &window);
    pthread_mutex_unlock(&s_windows_lock);
}

void Window::unregister_window(wid_t wid) {
    pthread_mutex_lock(&s_windows_lock);
    s_windows.remove(wid);
    pthread_mutex_unlock(&s_windows_lock);
}

Maybe<SharedPtr<Window>> Window::find_by_wid(wid_t wid) {
    pthread_mutex_lock(&s_windows_lock);
    auto result = s_windows.get(wid);
    pthread_mutex_unlock(&s_windows_lock);

    if (!result) {
        return {};
    }
    return { (*result)->shared_from_this() };
}

Window::~Window() {
    unregister_window(wid());
}

Window::Window(int x, int y, int width, int height, String name, bool has_alpha, WindowServer::WindowType type, wid_t parent_id)
    : m_parent_wid(parent_id), m_visible(type != WindowServer::WindowType::Frameless), m_has_alpha(has_alpha) {
    m_platform_window = Application::the().create_window(*this, x, y, width, height, move(name), has_alpha, type, parent_id);
    register_window(*this);
}

void Window::initialize() {
    on<WindowCloseEvent>([this](auto&) {
        m_removed = true;
        Application::the().main_event_loop().set_should_exit(true);
    });

    on<WindowDidResizeEvent>([this](auto&) {
        m_platform_window->did_resize();
        if (auto* main_widget = m_main_widget.get()) {
            main_widget->set_positioned_rect(rect());
        }
        pixels()->clear(Application::the().palette()->color(Palette::Background));
        invalidate_rect(rect());
    });

    on<WindowForceRedrawEvent>([this](auto&) {
        invalidate_rect(rect());
    });

    on<WindowStateEvent>([this](const WindowStateEvent& event) {
        auto& state_event = static_cast<const WindowStateEvent&>(event);
        if (state_event.active() == active()) {
            return;
        }

        if (!state_event.active()) {
            did_become_inactive();
        } else {
            did_become_active();
        }
        m_active = state_event.active();
    });

    on<MouseDownEvent, MouseDoubleEvent, MouseTripleEvent, MouseMoveEvent, MouseUpEvent, MouseScrollEvent>([this](const MouseEvent& event) {
        return handle_mouse_event(event);
    });

    on<KeyDownEvent, KeyUpEvent, TextEvent>([this](const Event& event) {
        return handle_key_or_text_event(event);
    });

    on<ThemeChangeEvent>([this](const ThemeChangeEvent& event) {
        pixels()->clear(Application::the().palette()->color(Palette::Background));
        invalidate_rect(rect());

        m_main_widget->dispatch(event);
    });

    Object::initialize();
}

bool Window::handle_mouse_event(const MouseEvent& event) {
    auto& mouse_event = static_cast<const MouseEvent&>(event);
    Widget* widget = nullptr;

    if (!mouse_event.mouse_move()) {
        hide_current_context_menu();
    }
    if (!mouse_event.button() && mouse_event.buttons_down()) {
        if (!focused_widget()) {
            return false;
        }
        widget = focused_widget().get();
    } else {
        widget = find_widget_at_point({ mouse_event.x(), mouse_event.y() });
        if (focused_widget() && focused_widget().get() != widget) {
            widget->dispatch(LeaveEvent {});
        }
        set_focused_widget(widget);
    }

    if (widget) {
        // FIXME: this is very suspicous now that MouseEvent's have distinct types.
        auto widget_relative_event = MouseEvent { mouse_event.name(),
                                                  mouse_event.buttons_down(),
                                                  mouse_event.x() - widget->positioned_rect().x(),
                                                  mouse_event.y() - widget->positioned_rect().y(),
                                                  mouse_event.z(),
                                                  mouse_event.button(),
                                                  mouse_event.modifiers() };
        return widget->dispatch(widget_relative_event);
    }
    return false;
}

bool Window::handle_key_or_text_event(const Event& event) {
    if (auto widget = focused_widget()) {
        return widget->dispatch(event);
    }
    return false;
}

void Window::set_rect(const Rect& rect) {
    m_rect = rect;
    if (m_main_widget) {
        m_main_widget->set_positioned_rect(rect);
    }
}

void Window::hide() {
    if (!m_visible) {
        return;
    }

    m_visible = false;
    m_platform_window->do_set_visibility(0, 0, false);
}

void Window::show(int x, int y) {
    if (m_visible) {
        return;
    }

    m_visible = true;
    m_platform_window->do_set_visibility(x, y, true);
}

void Window::hide_current_context_menu() {
    auto maybe_context_menu = m_current_context_menu.lock();
    if (maybe_context_menu) {
        maybe_context_menu->hide();
    }
}

Widget* Window::find_widget_at_point(Point p) {
    Widget* parent = m_main_widget.get();
    while (parent && !parent->children().empty()) {
        bool found = false;
        for (auto& child : parent->children()) {
            if (child->is_widget()) {
                auto& widget_child = const_cast<Widget&>(static_cast<const Widget&>(*child));
                if (!widget_child.hidden() && widget_child.positioned_rect().intersects(p)) {
                    parent = &widget_child;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            break;
        }
    }
    return parent;
}

void Window::set_focused_widget(Widget* widget) {
    if (!widget) {
        m_focused_widget.reset();
        return;
    }

    m_focused_widget = widget->weak_from_this();

    EventLoop::queue_event(widget->weak_from_this(), make_unique<FocusedEvent>());
}

SharedPtr<Widget> Window::focused_widget() {
    auto ret = m_focused_widget.lock();
    if (!ret) {
        set_focused_widget(nullptr);
    }
    return ret;
}

void Window::clear_current_context_menu() {
    m_current_context_menu.reset();
}

void Window::set_current_context_menu(ContextMenu* menu) {
    m_current_context_menu = menu->weak_from_this();
}

void Window::draw() {
    if (m_main_widget && !m_main_widget->hidden()) {
        m_main_widget->render();
        m_platform_window->flush_pixels();
    }
}

void Window::invalidate_rect(const Rect& rect) {
    if (rect.width() == 0 || rect.height() == 0) {
        return;
    }

    if (m_will_draw_soon) {
        return;
    }

    m_will_draw_soon = true;
    deferred_invoke([this] {
        draw();
        m_will_draw_soon = false;
    });
}
}
