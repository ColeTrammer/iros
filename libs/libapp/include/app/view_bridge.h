#pragma once

#include <app/forward.h>
#include <app/view_bridge_interface.h>
#include <graphics/forward.h>

namespace App {
class ViewBridge {
public:
    virtual ~ViewBridge() {}

    // iros reflect begin
    virtual ModelItem* item_at_position(const Point& point) = 0;
    // iros reflect end

    virtual void invalidate_all() = 0;

    virtual void install_model_listeners(Model&) {}
    virtual void uninstall_model_listeners(Model&) {}
};
}
