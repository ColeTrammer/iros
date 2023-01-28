#pragma once

#include <app/forward.h>
#include <app/tree_view_bridge_interface.h>

namespace App {
class TreeViewBridge {
public:
    virtual ~TreeViewBridge() {}

    // iros reflect begin
    virtual void render_item(const App::TreeViewItem& item) = 0;
    // iros reflect end
};
}
