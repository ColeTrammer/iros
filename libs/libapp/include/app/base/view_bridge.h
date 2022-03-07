#pragma once

#include <app/base/view_bridge_interface.h>
#include <app/forward.h>
#include <graphics/forward.h>

namespace App::Base {
class ViewBridge {
public:
    virtual ~ViewBridge() {}

    // os_2 reflect begin
    virtual ModelItem* item_at_position(const Point& point) = 0;
    // os_2 reflect end

    virtual void invalidate_all() = 0;

    virtual void install_model_listeners(Model&) {}
    virtual void uninstall_model_listeners(Model&) {}
};
}
