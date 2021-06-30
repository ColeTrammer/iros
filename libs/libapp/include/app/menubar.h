#pragma once

#include <app/forward.h>
#include <app/widget.h>
#include <liim/string.h>

namespace App {
class Menubar : public Widget {
    APP_OBJECT(Menubar)

public:
    Menubar();
    virtual void initialize() override;
    virtual ~Menubar() override;

    ContextMenu& create_menu(String name);
};
}
