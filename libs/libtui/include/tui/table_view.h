#pragma once

#include <tui/view.h>

namespace TUI {
class TableView : public View {
    APP_OBJECT(TableView)

public:
    virtual void render() override;
    virtual App::ModelItem* item_at_position(const Point&) override { return nullptr; }
};
}
