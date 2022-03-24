#pragma once

#include <app/base/tree_view_bridge.h>
#include <app/base/tree_view_interface.h>
#include <app/base/view.h>

namespace App::Base {
struct TreeViewItem {
    String name;
    Rect item_rect;
    Rect container_rect;
    ModelItem* item { nullptr };
    int level { 0 };
    bool open { false };
    Vector<TreeViewItem> children;
};

class TreeView : public View {
    APP_OBJECT(TreeView)

    APP_BASE_TREE_VIEW_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    TreeView(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<ViewBridge> view_bridge,
             SharedPtr<ScrollComponentBridge> scroll_component_bridge, SharedPtr<TreeViewBridge> tree_view_bridge);
    virtual void initialize() override;
    virtual ~TreeView() override;

    // os_2 reflect begin
    void render_items();

    void set_name_column(int col) { m_name_column = col; }
    int name_column() const { return m_name_column; }

    void set_padding(int padding) { m_padding = padding; }
    int padding() const { return m_padding; }

    void set_row_height(int row_height) { m_row_height = row_height; }
    int row_height() const { return m_row_height; }
    // os_2 reflect end

    TreeViewBridge& bridge() { return *m_bridge; }
    const TreeViewBridge& bridge() const { return *m_bridge; }

    TreeViewItem* internal_item_at_position(const Point& point);

private:
    virtual void install_model_listeners(Model& model) override;
    virtual void uninstall_model_listeners(Model& model) override;

    void rebuild_items();
    void rebuild_layout();

    Vector<TreeViewItem> m_items;
    SharedPtr<TreeViewBridge> m_bridge;
    int m_padding { 0 };
    int m_row_height { 12 };
    int m_name_column { 0 };
};
}
