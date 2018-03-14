#pragma once

namespace LuaCpp {

template <typename T>
class Pointer {
public:
    Pointer(T *obj, LuaRef lifetime)
        : _lifetime(std::move(lifetime)), _obj(obj) {}

    Pointer(LuaRef lifetime)
        : Pointer(nullptr, std::move(lifetime)) {}

    T *get() const {
        return _obj;
    }

    T *operator->() const {
        return _obj;
    }

    T &operator*() const {
        return *_obj;
    }

    operator bool() const {
        return _obj;
    }

    bool operator!() const {
        return !_obj;
    }

    friend bool operator==(std::nullptr_t, Pointer<T> const & ptr) {
        return nullptr == ptr._obj;
    }

    friend bool operator==(Pointer<T> const & ptr, std::nullptr_t) {
        return nullptr == ptr._obj;
    }

    friend bool operator!=(std::nullptr_t, Pointer<T> const & ptr) {
        return !(nullptr == ptr);
    }

    friend bool operator!=(Pointer<T> const & ptr, std::nullptr_t) {
        return !(nullptr == ptr);
    }

    friend bool operator==(Pointer<T> const & ptrA, Pointer<T> const & ptrB) {
        return ptrA.get() == ptrB.get();
    }

    friend bool operator!=(Pointer<T> const & ptrA, Pointer<T> const & ptrB) {
        return ptrA.get() != ptrB.get();
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
    struct is_primitive<LuaCpp::Pointer<T>> {
        static constexpr bool value = true;
    };

    template <typename T>
    inline LuaCpp::Pointer<T> _check_get(_id<LuaCpp::Pointer<T>>, lua_State *l, const int index) {
        auto result = _check_get(_id<T*>{}, l, index);
        if (result) {
            lua_pushvalue(l, index);
            LuaRef lifetime(l, luaL_ref(l, LUA_REGISTRYINDEX));
            return { result, lifetime };
        } else {
            return { LuaRef(l) };
        }
    }

    template <typename T>
    inline LuaCpp::Pointer<T> _get(_id<LuaCpp::Pointer<T>>, lua_State *l, const int index) {
        auto result = _get(_id<T*>{}, l, index);
        if (result) {
            lua_pushvalue(l, index);
            LuaRef lifetime(l, luaL_ref(l, LUA_REGISTRYINDEX));
            return { result, lifetime };
        } else {
            return { LuaRef(l) };
        }
    }

    template <typename T>
    inline void _push(lua_State *l, LuaCpp::Pointer<T> const & ptr) {
        ptr._push(l);
    }
}

}
