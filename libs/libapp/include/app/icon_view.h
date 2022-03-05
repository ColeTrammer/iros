#pragma once

#include <app/view.h>
#include <graphics/bitmap.h>
#include <graphics/rect.h>
#include <liim/vector.h>

namespace App {

class IconView : public View {
    APP_OBJECT(IconView)

public:
    virtual void initialize() override;
    virtual void render() override;

    void set_name_column(int col) { m_name_column = col; }

private:
    virtual ModelItem* item_at_position(const Point& point) override;
    virtual void install_model_listeners(Model& model) override;
    virtual void uninstall_model_listeners(Model& model) override;

    void rebuild_layout();
    void compute_layout();

    struct Item {
        SharedPtr<Bitmap> icon;
        String name;
        Rect rect;
        ModelItem* item;
    };

    Vector<Item> m_items;
    Point m_selection_start;
    Point m_selection_end;
    bool m_in_selection { false };
    int m_name_column { 0 };
    int m_icon_width { 32 };
    int m_icon_height { 32 };
    int m_icon_padding_x { 28 };
    int m_icon_padding_y { 8 };
};

}
