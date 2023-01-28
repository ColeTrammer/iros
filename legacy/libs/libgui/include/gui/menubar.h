#pragma once

#include <gui/widget.h>
#include <liim/string.h>

namespace GUI {
class Menubar : public Widget {
    APP_WIDGET(Widget, Menubar)

public:
    Menubar();
    virtual void did_attach() override;
    virtual ~Menubar() override;

    ContextMenu& create_menu(String name);
};
}
