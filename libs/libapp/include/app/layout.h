#pragma once

#include <liim/pointers.h>

namespace App {

class Widget;

class Layout {
public:
    virtual ~Layout() {}

    virtual void add(SharedPtr<Widget> widget) = 0;
    virtual void layout() = 0;

protected:
    Layout(Widget& widget) : m_widget(widget) {}

private:
    Widget& m_widget;
};

}
