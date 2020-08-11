#pragma once

#include <liim/pointers.h>
#include <liim/string.h>

class ProfileNode {
public:
    ProfileNode(String name, String object_path, uintptr_t address)
        : m_name(move(name)), m_object_path(move(object_path)), m_address(address) {}

    const Vector<ProfileNode>& nodes() const { return m_nodes; }

    const String& name() const { return m_name; }
    const String& object_path() const { return m_object_path; }

    void bump_total_count() { m_total_count++; }
    size_t total_count() const { return m_total_count; }

    void bump_self_count() {
        m_total_count++;
        m_self_count++;
    }
    size_t self_count() const { return m_self_count; }

    uintptr_t address() const { return m_address; }

    ProfileNode* link(const String& name, const String& object_path, uintptr_t address);
    void dump(int level) const;

private:
    Vector<ProfileNode> m_nodes;
    String m_name;
    String m_object_path;
    size_t m_total_count { 0 };
    size_t m_self_count { 0 };
    uintptr_t m_address { 0 };
};

class Profile {
public:
    static UniquePtr<Profile> create(const String& path);

    Profile(ProfileNode root) : m_root(move(root)) {}

    void dump() const;

private:
    ProfileNode m_root;
};

void view_profile(const String& path);
