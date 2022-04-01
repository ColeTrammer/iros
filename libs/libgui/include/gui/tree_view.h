#pragma once

#include <app/tree_view.h>
#include <gui/view.h>

namespace GUI {
class TreeView
    : public View
    , public App::TreeViewBridge {
    APP_WIDGET_BASE(App::TreeView, View, TreeView, self, self, self, self)

    APP_TREE_VIEW_INTERFACE_FORWARD(base())

public:
    TreeView();
    virtual void did_attach() override;
    virtual ~TreeView() override;

    // ^Widget
    virtual void render() override;

    // ^TreeViewBridge
    virtual void render_item(const App::TreeViewItem& item) override;

    // ^ViewBridge
    virtual App::ModelItem* item_at_position(const Point& point) override;
};
}
