#pragma once

#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <sys/mman.h>
#include <window_server/message.h>

class PixelBuffer;

typedef uint64_t wid_t;

class Window {
public:
    static SharedPtr<Window> find_window_intersecting_point(SharedPtr<Window> window, const Point& p);
    static SharedPtr<Window> find_window_intersecting_rect(SharedPtr<Window> window, const Rect& r);
    static void set_parent(SharedPtr<Window> child, SharedPtr<Window> parent);

    Window(const Rect& rect, String title, int client_id, WindowServer::WindowType type);
    ~Window();

    Window(const Window& other) = delete;

    const Rect& rect() const { return m_rect; }
    const Rect& content_rect() const { return m_content_rect; }

    void set_position(int x, int y);

    int close_button_x() const { return rect().x() + rect().width() - 13; }
    int close_button_y() const { return rect().y() + 10; }
    int close_button_radius() const { return 6; }

    wid_t id() const { return m_id; }
    int client_id() const { return m_client_id; }

    const String& shm_path() const { return m_shm_path; }

    WindowServer::WindowType type() const { return m_type; }

    void update_content_from_rect();
    void update_rect_from_content();
    void relative_resize(int delta_x, int delta_y);

    void swap();

    const String& title() const { return m_title; }
    void set_title(String title) { m_title = move(title); }

    SharedPtr<PixelBuffer>& buffer() { return m_front_buffer; }
    const SharedPtr<PixelBuffer>& buffer() const { return m_front_buffer; }

    Rect& resize_rect() { return m_resize_rect; }
    const Rect& resize_rect() const { return m_resize_rect; }

    void set_in_resize(bool b) { m_in_resize = b; }
    bool in_resize() const { return m_in_resize; }

    bool visible() const { return m_visible; }
    void set_visible(bool b) { m_visible = b; }

    bool resizable() const { return type() == WindowServer::WindowType::Application; }
    bool movable() const { return type() == WindowServer::WindowType::Application; }

    Vector<SharedPtr<Window>>& children() { return m_children; }
    const Vector<SharedPtr<Window>>& children() const { return m_children; }

    Window* parent() { return m_parent; }
    const Window* parent() const { return m_parent; }

    Window* root() {
        if (!parent()) {
            return this;
        }
        return parent()->root();
    }
    const Window* root() const { return const_cast<Window&>(*this).root(); }

    void did_remove();

private:
    void map_buffers();

    String m_shm_path;
    Rect m_rect;
    Rect m_content_rect;
    const wid_t m_id;
    String m_title;
    const int m_client_id;
    WindowServer::WindowType m_type;
    bool m_visible { true };
    bool m_in_resize { false };
    Rect m_resize_rect;
    Vector<SharedPtr<Window>> m_children;
    Window* m_parent { nullptr };
    SharedPtr<PixelBuffer> m_front_buffer;
    SharedPtr<PixelBuffer> m_back_buffer;
    void* m_raw_buffer { MAP_FAILED };
    size_t m_raw_buffer_size { 0 };
};
