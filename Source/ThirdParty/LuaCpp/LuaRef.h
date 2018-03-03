#pragma once

namespace LuaCpp {

namespace detail {
    class LuaRefDeleter {
    public:
        LuaRefDeleter(lua_State *l) : _l{l} {}

        void operator()(int *ref) const {
            luaL_unref(_l, LUA_REGISTRYINDEX, *ref);
            delete ref;
        }

    private:
        lua_State *_l;
    };
}

// Lua provides a registry, a predefined table that can be used by any C code to store whatever Lua values it needs to store.
// LuaRef store a value in the registry with a unique key
class LuaRef {
public:
    LuaRef() {}
    LuaRef(lua_State *l, int ref)
        : _ref(new int{ref}, detail::LuaRefDeleter{l}) {}

    LuaRef(lua_State *l)
        : LuaRef(l, LUA_REFNIL) {}

    void Push(lua_State *l) const {
        // Push a value from Lua registry
        lua_rawgeti(l, LUA_REGISTRYINDEX, *_ref);
    }

private:
    std::shared_ptr<int> _ref;
};

// Store a value in the registry
template <typename T>
LuaRef make_Ref(lua_State *l, T &&value) {
    // Push a value to save it in the registry
    detail::_push(l, std::forward<T>(value));
    // Store it in the registry and pop.
    // 'ref' means a unique key for the registry table
    return LuaRef(l, luaL_ref(l, LUA_REGISTRYINDEX));
}

namespace detail {
    inline void append_ref_recursive(lua_State *, std::vector<LuaRef> &) {}

    template <typename Head, typename... Tail>
    void append_ref_recursive(lua_State *l, std::vector<LuaRef> &refs, Head&& head, Tail&&... tail) {
        refs.push_back(make_Ref(l, std::forward<Head>(head)));

        append_ref_recursive(l, refs, std::forward<Tail>(tail)...);
    }
}

// Store multiple values in Lua registry
template <typename... Values>
std::vector<LuaRef> make_Refs(lua_State *l, Values&&... args) {
    std::vector<LuaRef> refs;
    refs.reserve(sizeof...(Values));

    detail::append_ref_recursive(l, refs, std::forward<Values>(args)...);
    return refs;
}

}
