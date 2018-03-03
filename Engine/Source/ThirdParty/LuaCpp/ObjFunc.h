#pragma once

namespace LuaCpp {

template <int N, typename Ret, typename... Args>
class ObjFunc : public BaseFunc {
    using _func_type = std::function<Ret(Args...)>;

public:
    ObjFunc(lua_State *l, const std::string &name, Ret(*func)(Args...))
        : ObjFunc(l, name, _func_type(func)) {}

    // Set function field of metatable of object
    ObjFunc(lua_State *l, const std::string &name, _func_type func) 
        : _name(name), _func(func) {
        // NOTE: Assume that metatable of object is already pushed on top
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        // Add a closure function with name
        lua_setfield(l, -2, name.c_str());
    }

    // Each application of a function receives a new Lua context so this argument is necessary.
    virtual int Invoke(lua_State *l) override {
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        // Push the returned value(s) onto the stack.
        detail::_push(l, detail::_lift(_func, args));
        // Return the number of values returned so Lua knows how many values to pop on its end.
        return N;
    }

private:
    _func_type _func;
    std::string _name;
};

template <typename... Args>
class ObjFunc<0, void, Args...> : public BaseFunc {
    using _func_type = std::function<void(Args...)>;

public:
    ObjFunc(lua_State *l, const std::string &name, void(*func)(Args...))
        : ObjFunc(l, name, _func_type(func)) {}

    // Set function field of metatable of object
    ObjFunc(lua_State *l, const std::string &name, _func_type func) 
        : _name(name), _func(func) {
        // NOTE: Assume that metatable of object is already pushed on top
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        // Add a closure function with name
        lua_setfield(l, -2, name.c_str());
    }

    // Each application of a function receives a new Lua context so this argument is necessary.
    virtual int Invoke(lua_State *l) override {
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        detail::_lift(_func, args);
        // There are no return values
        return 0;
    }

private:
    _func_type _func;
    std::string _name;
};

}
