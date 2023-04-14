#pragma once

namespace iris {
class UserspaceAccessEnabler {
public:
    UserspaceAccessEnabler();

    ~UserspaceAccessEnabler();

private:
    bool m_has_smap { false };
};
}
