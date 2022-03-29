#pragma once

#include <app/base/tree_view.h>
#include <tui/view.h>

namespace TUI {
class TreeView
    : public View
    , public App::Base::TreeViewBridge {
    APP_WIDGET_BASE(App::Base::TreeView, View, TreeView, self, self, self, self)

    APP_BASE_TREE_VIEW_INTERFACE_FORWARD(base())

public:
    TreeView();
    virtual void did_attach() override;
    virtual ~TreeView() override;

    // ^Panel
    virtual void render() override;

    // ^App::Base::TreeViewBridge
    virtual void render_item(const App::Base::TreeViewItem& item) override;

    // ^App::Base::ViewBridge
    virtual App::ModelItem* item_at_position(const Point& point) override;
};
}
