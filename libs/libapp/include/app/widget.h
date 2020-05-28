#pragma once

#include <app/object.h>
#include <graphics/rect.h>

namespace App {

class Layout;
class Window;

class Widget : public Object {
public:
    template<typename... Args>
    static SharedPtr<Widget> create(SharedPtr<Object> parent, Args... args) {
        auto ret = SharedPtr<Widget>(new Widget(forward<Args>(args)...));
        if (parent) {
            parent->add_child(ret);
        }
        return ret;
    }

    virtual ~Widget() override;

    virtual void render();

    void set_rect(Rect rect) { m_rect = move(rect); }
    const Rect& rect() const { return m_rect; }

    Window* window();

    Layout* layout() { return m_layout.get(); }
    const Layout* layout() const { return m_layout.get(); }

    template<typename LayoutClass, typename... Args>
    Layout& set_layout(Args... args) {
        return *(m_layout = make_unique<LayoutClass>(*this, forward<Args>(args)...));
    }

    bool hidden() const { return m_hidden; }
    void set_hidden(bool b) { m_hidden = b; }

protected:
    Widget();

private:
    virtual bool is_widget() const final { return true; }

    Rect m_rect;
    UniquePtr<Layout> m_layout;
    bool m_hidden { false };
};

}
