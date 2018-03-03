#pragma once

namespace LuaCpp {

template <typename T>
class Reference {
public:
    Reference(T &obj, LuaRef lifetime) 
        : _lifetime(std::move(lifetime)), _obj(&obj) {}

    T &get() const { 
        return *_obj; 
    }

    operator T&() const { 
        return get(); 
    }

    void _push(lua_State *l) const { 
        _lifetime.Push(l); 
    }

private:
    LuaRef _lifetime;
    T *_obj;
};

namespace detail {
    template <typename T>
    struct is_primitive<LuaCpp::Reference<T>> {
        static constexpr bool value = true;
    };

    template <typename T>
    inline LuaCpp::Reference<T> _check_get(_id<LuaCpp::Reference<T>>, lua_State *l, const int index) {
        T &result = _check_get(_id<T&>{}, l, index);
        lua_pushvalue(l, index);
        LuaRef lifetime(l, luaL_ref(l, LUA_REGISTRYINDEX));
        return { result, lifetime };
    }

    template <typename T>
    inline LuaCpp::Reference<T> _get(_id<LuaCpp::Reference<T>>, lua_State *l, const int index) {
        T &result = _get(_id<T&>{}, l, index);
        lua_pushvalue(l, index);
        LuaRef lifetime(l, luaL_ref(l, LUA_REGISTRYINDEX));
        return { result, lifetime };
    }

    template <typename T>
    inline void _push(lua_State *l, LuaCpp::Reference<T> const & ref) {
        ref._push(l);
    }
}

}
