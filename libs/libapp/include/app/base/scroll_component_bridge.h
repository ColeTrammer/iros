#pragma once

#include <app/base/scroll_component_bridge_interface.h>

namespace App::Base {
class ScrollComponentBridge {
public:
    virtual ~ScrollComponentBridge() {}

    // os_2 reflect begin
    virtual int scrollbar_width() const = 0;
    // os_2 reflect end
};
}
