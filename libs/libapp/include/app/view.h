#pragma once

#include <app/forward.h>
#include <app/scroll_component.h>
#include <app/selection.h>
#include <app/view_bridge.h>
#include <app/view_interface.h>
#include <app/widget.h>

APP_EVENT(App, ViewRootChanged, Event, (), (), ());
APP_EVENT(App, ViewItemActivated, Event, (), ((ModelItem*, item)), ())

namespace App {
class View
    : public Widget
    , public ScrollComponent {
    APP_OBJECT(View)

    APP_EMITS(Widget, ViewRootChanged, ViewItemActivated)

    APP_VIEW_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    virtual void initialize() override;
    virtual ~View() override;

    // iros reflect begin
    App::Model* model() { return m_model.get(); }
    const App::Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<App::Model> model);

    App::ModelItem* hovered_item() const { return m_hovered_item; }
    void set_hovered_item(App::ModelItem* item);

    App::Selection& selection() { return m_selection; }
    const App::Selection& selection() const { return m_selection; }

    App::ModelItem* root_item() { return m_root_item; }
    const App::ModelItem* root_item() const { return m_root_item; }

    void set_root_item(App::ModelItem* item);
    // iros reflect end

    const ViewBridge& bridge() const { return *m_bridge; }
    ViewBridge& bridge() { return *m_bridge; }

protected:
    explicit View(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<ViewBridge> view_bridge,
                  SharedPtr<ScrollComponentBridge> scroll_component_bridge);

    virtual void install_model_listeners(Model& model);
    virtual void uninstall_model_listeners(Model& model);

private:
    SharedPtr<Model> m_model;
    SharedPtr<ViewBridge> m_bridge;
    ModelItem* m_hovered_item;
    Selection m_selection;
    ModelItem* m_root_item { nullptr };
};
}
