#include <app/button.h>
#include <app/context_menu.h>
#include <app/flex_layout_engine.h>
#include <app/menubar.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

constexpr int menubar_height = 20;

namespace App {
class MenubarItem final : public Widget {
    APP_OBJECT(MenubarItem)

public:
    explicit MenubarItem(String name) : m_name(move(name)) {}
    virtual void initialize() override {
        m_menu = ContextMenu::create(shared_from_this(), parent_window()->shared_from_this());
        m_button = Button::create(shared_from_this(), m_name);
        m_button->on<App::ClickEvent>(*this, [this](auto&) {
            if (m_menu->menu_items().empty()) {
                return;
            }

            if (!m_shown) {
                m_menu->show({ positioned_rect().x(), positioned_rect().y() + positioned_rect().height() });
                m_shown = true;
            } else {
                m_shown = false;
            }
        });

        on<ResizeEvent>([this](const ResizeEvent&) {
            m_button->set_positioned_rect(positioned_rect());
        });
    }
    virtual ~MenubarItem() override {}

    ContextMenu& menu() { return *m_menu; }

private:
    SharedPtr<ContextMenu> m_menu;
    SharedPtr<Button> m_button;
    String m_name;
    bool m_shown { false };
};

Menubar::Menubar() {}

void Menubar::initialize() {
    set_layout_constraint({ LayoutConstraint::AutoSize, menubar_height });

    auto& layout = set_layout_engine<HorizontalFlexLayoutEngine>();
    layout.set_margins({ 0, 0, 0, 0 });
    layout.set_spacing(0);
}

ContextMenu& Menubar::create_menu(String name) {
    auto& item = layout_engine()->add<MenubarItem>(name);
    return item.menu();
}

Menubar::~Menubar() {}
}
