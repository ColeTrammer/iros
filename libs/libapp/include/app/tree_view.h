#pragma once

#include <app/base/tree_view.h>
#include <app/view.h>

namespace App {
class TreeView
    : public View
    , public Base::TreeViewBridge {
    APP_WIDGET_BASE(Base::TreeView, View, TreeView, self, self, self, self)

    APP_BASE_TREE_VIEW_INTERFACE_FORWARD(base())

public:
    TreeView();
    virtual void did_attach() override;
    virtual ~TreeView() override;

    // ^Widget
    virtual void render() override;

    // ^TreeViewBridge
    virtual void render_item(const Base::TreeViewItem& item) override;

    // ^ViewBridge
    virtual ModelItem* item_at_position(const Point& point) override;
};
}
