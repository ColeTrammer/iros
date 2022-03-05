#pragma once

#include <app/scroll_component.h>
#include <app/view.h>

namespace App {
class TreeView
    : public View
    , public ScrollComponent {
    APP_OBJECT(TreeView)

public:
    TreeView() : ScrollComponent(static_cast<Object&>(*this)) {}
    virtual void initialize() override;
    virtual void render() override;

    void set_name_column(int col) { m_name_column = col; }

private:
    struct Item {
        String name;
        Rect item_rect;
        Rect container_rect;
        ModelItem* item { nullptr };
        int level { 0 };
        bool open { false };
        Vector<Item> children;
    };

    virtual ModelItem* item_at_position(const Point& point) override;
    virtual void install_model_listeners(Model& model) override;
    virtual void uninstall_model_listeners(Model& model) override;

    Item* internal_item_at_position(const Point& point);

    void rebuild_items();
    void rebuild_layout();

    Vector<Item> m_items;
    int m_padding { 2 };
    int m_row_height { 32 };
    int m_name_column { 0 };
};
}
