#pragma once

#include <app/view.h>

namespace App {
class TreeView : public View {
    APP_OBJECT(TreeView)

public:
    virtual void initialize() override;
    virtual void render() override;

    void set_name_column(int col) { m_name_column = col; }

private:
    virtual ModelItem* item_at_position(int, int) override;
    virtual void install_model_listeners(Model& model) override;
    virtual void uninstall_model_listeners(Model& model) override;

    void rebuild_layout();

    struct Item {
        String name;
        Rect item_rect;
        Rect container_rect;
        ModelItem* item;
        int level { 0 };
        bool open { true };
        Vector<Item> children;
    };

    Vector<Item> m_items;
    int m_padding { 2 };
    int m_row_height { 32 };
    int m_name_column { 0 };
};
}
