#pragma once

#include <app/scroll_component_bridge_interface.h>

namespace App {
class ScrollComponentBridge {
public:
    virtual ~ScrollComponentBridge() {}

    // iros reflect begin
    virtual int scrollbar_width() const = 0;
    // iros reflect end
};
}
