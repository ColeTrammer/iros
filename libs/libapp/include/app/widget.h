#pragma once

#include <graphics/rect.h>
#include <app/object.h>

namespace App {

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

protected:
    Widget();

private:
    virtual bool is_widget() const final { return true; }

    Rect m_rect;
};

}
