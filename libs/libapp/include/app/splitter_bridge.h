#include <app/splitter_bridge_interface.h>

namespace App {
class SplitterBridge {
public:
    virtual ~SplitterBridge() {}

    // iros reflect begin
    virtual int gutter_width() const = 0;
    // iros reflect end
};
}
