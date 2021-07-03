#pragma once

#include <app/forward.h>
#include <eventloop/object.h>
#include <graphics/bitmap.h>
#include <liim/hash_map.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <sys/mman.h>
#include <window_server/message.h>

namespace App {
class PlatformWindow {
public:
    PlatformWindow() {}
    virtual ~PlatformWindow() {}

    virtual SharedPtr<Bitmap> pixels() = 0;
    virtual void flush_pixels() = 0;
    virtual void did_resize() = 0;
    virtual void do_set_visibility(int x, int y, bool visible) = 0;
};

class Window : public Object {
    APP_OBJECT(Window);

public:
    static Maybe<SharedPtr<Window>> find_by_wid(wid_t wid);
    static void for_each_window(Function<void(Window&)>);

    virtual ~Window();

    SharedPtr<Bitmap> pixels() { return m_platform_window->pixels(); }

    wid_t wid() const { return m_wid; }

    void set_focused_widget(Widget* widget);
    SharedPtr<Widget> focused_widget();

    void invalidate_rect(const Rect& rect);

    template<typename T, typename... Args>
    T& set_main_widget(Args... args) {
        auto ret = T::create(shared_from_this(), forward<Args>(args)...);
        ret->set_positioned_rect(rect());
        set_focused_widget(ret.get());
        invalidate_rect(rect());
        m_main_widget = ret;
        return *ret;
    }

    const Rect& rect() const { return m_rect; }
    void set_rect(const Rect& rect);

    void set_current_context_menu(ContextMenu* menu);
    void clear_current_context_menu();

    void hide();
    void show(int x, int y);
    bool visible() const { return m_visible; }
    bool active() const { return m_active; }
    bool has_alpha() const { return m_has_alpha; }
    bool removed() const { return m_removed; }

    PlatformWindow& platform_window() { return *m_platform_window; }

    wid_t parent_wid() const { return m_parent_wid; }

    void set_id(wid_t id) {
        assert(m_wid == 0);
        m_wid = id;
    }

protected:
    Window(int x, int y, int width, int height, String name, bool has_alpha = false,
           WindowServer::WindowType window_type = WindowServer::WindowType::Application, wid_t parent_id = 0);
    virtual void on_event(const Event& event) override;

    virtual void did_become_active() {}
    virtual void did_become_inactive() {}

private:
    virtual bool is_window() const final { return true; }

    Widget* find_widget_at_point(Point p);
    void draw();
    void hide_current_context_menu();

    static void register_window(Window& window);
    static void unregister_window(wid_t wid);

    wid_t m_wid { 0 };
    wid_t m_parent_wid { 0 };
    WeakPtr<Widget> m_focused_widget;
    WeakPtr<ContextMenu> m_current_context_menu;
    SharedPtr<Widget> m_main_widget;
    UniquePtr<PlatformWindow> m_platform_window;
    Rect m_rect;
    bool m_will_draw_soon { false };
    bool m_visible { true };
    bool m_active { false };
    bool m_has_alpha { false };
    bool m_removed { false };
};
}
