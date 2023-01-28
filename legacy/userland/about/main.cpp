#include <app/flex_layout_engine.h>
#include <assert.h>
#include <eventloop/event.h>
#include <eventloop/object.h>
#include <gui/application.h>
#include <gui/button.h>
#include <gui/context_menu.h>
#include <gui/menubar.h>
#include <gui/text_label.h>
#include <gui/widget.h>
#include <gui/window.h>
#include <liim/format.h>
#include <unistd.h>

class TestWidget : public GUI::Widget {
    APP_WIDGET(GUI::Widget, TestWidget)

public:
    TestWidget() {}

    virtual void did_attach() override {
        on<App::TextEvent>([](const App::TextEvent& event) {
            out_log("typed: '{}'", event.text());
            return true;
        });

        Widget::did_attach();
    }
};

int main() {
    auto app = GUI::Application::create();

    auto window = GUI::Window::create(nullptr, 300, 300, 250, 250, "About");
    auto& main_widget = window->set_main_widget<GUI::Widget>();

    auto& root_layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();
    root_layout.set_margins({ 0, 0, 0, 0 });
    root_layout.set_spacing(0);

    auto& menubar = root_layout.add<GUI::Menubar>();

    menubar.create_menu("File");

    auto& edit_menu = menubar.create_menu("Edit");
    edit_menu.add_menu_item("Edit", [] {
        printf("Editing\n");
    });

    auto& help_menu = menubar.create_menu("Help");
    help_menu.add_menu_item("Help", [] {
        printf("Helping\n");
    });

    auto& label = root_layout.add<GUI::TextLabel>("Hello World!");
    root_layout.add<GUI::Widget>();
    root_layout.add<TestWidget>();
    auto& button = root_layout.add<GUI::Button>("Click Me!");
    button.on<GUI::ClickEvent>({}, [](auto&) {
        printf("clicked!\n");
    });

    auto context_menu = GUI::ContextMenu::create(window.get(), window);
    context_menu->add_menu_item("A", [] {
        printf("A Pressed\n");
    });
    context_menu->add_menu_item("B", [] {
        printf("B Pressed\n");
    });
    context_menu->add_menu_item("C", [] {
        printf("C Pressed\n");
    });
    label.set_context_menu(context_menu);

    app->enter();
    return 0;
}
