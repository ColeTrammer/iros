#ifndef _BUILTIN_H
#define _BUILTIN_H 1

#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/string.h>

namespace Sh {
class BuiltInOperation {
public:
    BuiltInOperation(String name, Function<int(int, char**)> entry) : m_name(move(name)), m_entry(move(entry)) {}

    const String& name() const { return m_name; }
    int execute(int argc, char** argv) { return m_entry(argc, argv); }

private:
    String m_name;
    Function<int(int, char**)> m_entry;
};

class BuiltInManager {
public:
    static BuiltInManager& the();

    void register_builtin(String name, Function<int(int, char**)> entry);
    void unregister_builtin(const String& name);

    BuiltInOperation* find(const String& name);
    const HashMap<String, BuiltInOperation>& builtins() const { return m_builtins; }

private:
    HashMap<String, BuiltInOperation> m_builtins;
};
}

#define __CAT_(x, y) x##y
#define __CAT(x, y)  __CAT_(x, y)

#define SH_REGISTER_BUILTIN(name, entry)                                         \
    __attribute__((constructor)) void __CAT(__register_##entry, __COUNTER__)() { \
        Sh::BuiltInManager::the().register_builtin("" #name, entry);             \
    }

class FunctionBody;

extern HashMap<String, String> g_aliases;
extern HashMap<String, FunctionBody> g_functions;

#endif /* _BUILTIN_H */
