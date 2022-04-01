#include <app/flex_layout_engine.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>
#include <gui/button.h>
#include <gui/context_menu.h>
#include <gui/menubar.h>
#include <gui/widget.h>
#include <gui/window.h>

constexpr int menubar_height = 20;

namespace GUI {
class MenubarItem final : public Widget {
    APP_WIDGET(Widget, MenubarItem)

public:
    explicit MenubarItem(String name) : m_name(move(name)) {}

    virtual void did_attach() override {
        m_menu = add<ContextMenu>(parent_window()->shared_from_this());

        auto& button = create_widget<Button>(m_name);
        listen<ClickEvent>(button, [this, &button](auto&) {
            if (menu().menu_items().empty()) {
                return;
            }

            if (!m_shown) {
                menu().show({ positioned_rect().x(), positioned_rect().y() + positioned_rect().height() });
                m_shown = true;
            } else {
                m_shown = false;
            }
        });

        on<App::ResizeEvent>([this, &button](const App::ResizeEvent&) {
            button.set_positioned_rect(positioned_rect());
        });

        Widget::did_attach();
    }
    virtual ~MenubarItem() override {}

    ContextMenu& menu() { return *m_menu; }

    SharedPtr<ContextMenu> m_menu;
    String m_name;
    bool m_shown { false };
};

Menubar::Menubar() {}

void Menubar::did_attach() {
    set_layout_constraint({ App::LayoutConstraint::AutoSize, menubar_height });

    auto& layout = set_layout_engine<App::HorizontalFlexLayoutEngine>();
    layout.set_margins({ 0, 0, 0, 0 });
    layout.set_spacing(0);
}

ContextMenu& Menubar::create_menu(String name) {
    auto& item = layout_engine()->add<MenubarItem>(name);
    return item.menu();
}

Menubar::~Menubar() {}
}
