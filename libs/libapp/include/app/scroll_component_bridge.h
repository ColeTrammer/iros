#pragma once

#include <app/scroll_component_bridge_interface.h>

namespace App {
class ScrollComponentBridge {
public:
    virtual ~ScrollComponentBridge() {}

    // os_2 reflect begin
    virtual int scrollbar_width() const = 0;
    // os_2 reflect end
};
}
