#pragma once

#include <app/tree_view.h>
#include <tui/view.h>

namespace TUI {
class TreeView
    : public View
    , public App::TreeViewBridge {
    APP_WIDGET_BASE(App::TreeView, View, TreeView, self, self, self, self)

    APP_TREE_VIEW_INTERFACE_FORWARD(base())

public:
    TreeView();
    virtual void did_attach() override;
    virtual ~TreeView() override;

    // ^Panel
    virtual void render() override;

    // ^App::TreeViewBridge
    virtual void render_item(const App::TreeViewItem& item) override;

    // ^App::ViewBridge
    virtual App::ModelItem* item_at_position(const Point& point) override;
};
}
