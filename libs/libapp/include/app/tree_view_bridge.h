#pragma once

#include <app/forward.h>
#include <app/tree_view_bridge_interface.h>

namespace App {
class TreeViewBridge {
public:
    virtual ~TreeViewBridge() {}

    // os_2 reflect begin
    virtual void render_item(const App::TreeViewItem& item) = 0;
    // os_2 reflect end
};
}
