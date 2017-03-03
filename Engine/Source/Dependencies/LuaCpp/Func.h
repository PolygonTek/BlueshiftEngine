#pragma once

namespace LuaCpp {

template <int N, typename Ret, typename... Args>
class Func : public BaseFunc {
    using _func_type = std::function<Ret(detail::decay_if_primitive<Args>...)>;

public:
    // Push a function to be used
    Func(lua_State *&l, _func_type func) 
        : _func(func) {
        // Push this pointer for use in the closure
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        // Push our dispatcher with the above upvalue
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
    }

    // Each application of a function receives a new Lua context so
    // this argument is necessary.
    virtual int Invoke(lua_State *l) override {
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<detail::decay_if_primitive<Args>...> args = detail::_get_args<detail::decay_if_primitive<Args>...>(l);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        // Push the returned value(s) onto the stack.
        detail::_push(l, detail::_lift(_func, args));
        // Return the number of values returned so Lua knows how many values to pop on its end.
        return N;
    }

private:
    _func_type _func;
};

template <typename... Args>
class Func<0, void, Args...> : public BaseFunc {
    using _func_type = std::function<void(detail::decay_if_primitive<Args>...)>;

public:
    // Push a function to be used
    Func(lua_State *&l, _func_type func) 
        : _func(func) {
        // Push this pointer for use in the closure
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        // Push our dispatcher with the above upvalue
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
    }

    // Each application of a function receives a new Lua context so
    // this argument is necessary.
    virtual int Invoke(lua_State *l) override {
        // Read all arguments from the stack into a tuple of type std::tuple<Args...>
        std::tuple<detail::decay_if_primitive<Args>...> args = detail::_get_args<detail::decay_if_primitive<Args>...>(l);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        detail::_lift(_func, args);
        // There are no return values
        return 0;
    }

private:
    _func_type _func;
};

}
