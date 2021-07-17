#include <app/application.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/context_menu.h>
#include <app/menubar.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <assert.h>
#include <eventloop/event.h>
#include <eventloop/object.h>
#include <unistd.h>

class TestWidget : public App::Widget {
    APP_OBJECT(TestWidget)

private:
    virtual void on_key_event(const App::KeyEvent& event) override {
        if (event.key_down()) {
            printf("pressed: '%s'\n", event.text().string());
        }
    }
};

int main() {
    auto app = App::Application::create();

    auto window = App::Window::create(nullptr, 300, 300, 250, 250, "About");
    auto& main_widget = window->set_main_widget<App::Widget>();

    auto& root_layout = main_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    root_layout.set_margins({ 0, 0, 0, 0 });
    root_layout.set_spacing(0);

    auto& menubar = root_layout.add<App::Menubar>();

    menubar.create_menu("File");

    auto& edit_menu = menubar.create_menu("Edit");
    edit_menu.add_menu_item("Edit", [] {
        printf("Editing\n");
    });

    auto& help_menu = menubar.create_menu("Help");
    help_menu.add_menu_item("Help", [] {
        printf("Helping\n");
    });

    auto& label = root_layout.add<App::TextLabel>("Hello World!");
    root_layout.add<App::Widget>();
    root_layout.add<TestWidget>();
    auto& button = root_layout.add<App::Button>("Click Me!");
    button.on_click = [] {
        printf("clicked!\n");
    };

    auto context_menu = App::ContextMenu::create(window, window);
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
