#pragma once

#include <liim/pointers.h>

namespace App {

class Widget;

class Layout {
public:
    Layout();
    virtual ~Layout() {}

    virtual void add(SharedPtr<Widget> widget) = 0;
    virtual void layout() = 0;

    Widget& widget() { return m_widget; }
    const Widget& widget() const { return m_widget; }

protected:
    Layout(Widget& widget) : m_widget(widget) {}

private:
    Widget& m_widget;
};

}
