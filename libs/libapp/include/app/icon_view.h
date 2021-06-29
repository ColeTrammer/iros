#pragma once

#include <app/view.h>
#include <graphics/bitmap.h>
#include <graphics/rect.h>
#include <liim/vector.h>

namespace App {

class IconView : public View {
    APP_OBJECT(IconView)

public:
    virtual void render() override;
    virtual void on_mouse_down(MouseEvent& event);
    virtual void on_mouse_move(MouseEvent& event);
    virtual void on_mouse_up(MouseEvent& event);
    virtual void on_resize() override;
    virtual void model_did_update() override;

    void set_name_column(int col) { m_name_column = col; }

private:
    virtual ModelIndex index_at_position(int, int) override;

    void compute_layout();

    struct Item {
        SharedPtr<Bitmap> icon;
        String name;
        Rect rect;
        ModelIndex index;
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
