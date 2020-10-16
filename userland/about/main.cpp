#include <app/app.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/context_menu.h>
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
    virtual void on_key_event(App::KeyEvent& event) override {
        if (event.key_down()) {
            printf("pressed: '%c'\n", event.ascii());
        }
    }
};

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 300, 300, 250, 250, "About");
    auto& main_widget = window->set_main_widget<App::Widget>();

    auto& layout = main_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    auto& label = layout.add<App::TextLabel>("Hello World!");
    layout.add<App::Widget>();
    layout.add<TestWidget>();
    auto& button = layout.add<App::Button>("Click Me!");
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

    app.enter();
    return 0;
}
