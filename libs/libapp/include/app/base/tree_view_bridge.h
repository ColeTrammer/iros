#pragma once

#include <app/base/tree_view_bridge_interface.h>
#include <app/forward.h>

namespace App::Base {
class TreeViewBridge {
public:
    virtual ~TreeViewBridge() {}

    // os_2 reflect begin
    virtual void render_item(const App::Base::TreeViewItem& item) = 0;
    // os_2 reflect end
};
}
