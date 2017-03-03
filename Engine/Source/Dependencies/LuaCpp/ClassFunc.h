#pragma once

namespace LuaCpp {

template <int N, typename T, typename Ret, typename... Args>
class ClassFunc : public BaseFunc {
    using _func_type = std::function<Ret(T*, Args...)>;

public:
    ClassFunc(lua_State *l, const std::string &name, const std::string &metatableName, Ret(*func)(Args...))
        : ClassFunc(l, name, _func_type(func)) {}

    // Set function field of metatable of class T
    ClassFunc(lua_State *l, const std::string &name, const std::string &metatableName, _func_type func)
        : _func(func), _name(name), _metatable_name(metatableName) {
        // NOTE: Assume that metatable of class T is already pushed on top
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        // Add a closure function with name
        lua_setfield(l, -2, name.c_str());
    }

    virtual int Invoke(lua_State *l) override {
        T *self = _get_self(l);
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        // Pack self and function arguments to one tuple
        std::tuple<T*, Args...> pack = std::tuple_cat(std::tie(self), args);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        // Push the returned value(s) onto the stack.
        detail::_push(l, detail::_lift(_func, pack));
        // Return the number of values returned so Lua knows how many values to pop on its end.
        return N;
    }

    virtual BaseFunc *Clone(lua_State *l, const std::string &metatableName) override {
        return new ClassFunc(l, _name, metatableName, _func);
    }

private:
    T *_get_self(lua_State *l) {
        // Check whether first element is a userdata that has the metatable named _metatable_name
        // If so it returns the userdata address or returns nullptr
        detail::UserData *userdata = (detail::UserData *)luaL_checkudata(l, 1, _metatable_name.c_str());
        T *self = (T *)userdata->addr;
        // Remove it from the stack
        lua_remove(l, 1);
        return self;
    }

    _func_type _func;
    std::string _name;
    std::string _metatable_name;
};

template <typename T, typename... Args>
class ClassFunc<0, T, void, Args...> : public BaseFunc {
    using _func_type = std::function<void(T*, Args...)>;

public:
    ClassFunc(lua_State *l, const std::string &name, const std::string &metatableName, void(*func)(Args...))
        : ClassFunc(l, name, metatableName, _func_type(func)) {}

    // Set function field of metatable of class T
    ClassFunc(lua_State *l, const std::string &name, const std::string &metatableName, _func_type func)
        : _func(func), _name(name), _metatable_name(metatableName) {
        // NOTE: Assume that metatable of class T is already pushed on top
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        // Add a closure function with name
        lua_setfield(l, -2, name.c_str());
    }

    virtual int Invoke(lua_State *l) override {
        T *self = _get_self(l);
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        // Pack self and function arguments to one tuple
        std::tuple<T*, Args...> pack = std::tuple_cat(std::tie(self), args);
        // Call the function '_func' with those arguments by converting the tuple back into a parameter pack.
        detail::_lift(_func, pack);
        // There are no return values
        return 0;
    }

    virtual BaseFunc *Clone(lua_State *l, const std::string &metatableName) override {
        return new ClassFunc(l, _name, metatableName, _func);
    }

private:
    T *_get_self(lua_State *l) {
        // Check whether first element is a userdata that has the metatable named _metatable_name
        // If so it returns the userdata address or returns nullptr
        detail::UserData *userdata = (detail::UserData *)luaL_checkudata(l, 1, _metatable_name.c_str());
        T *self = (T *)userdata->addr;
        // Remove it from the stack
        lua_remove(l, 1);
        return self;
    }

    _func_type _func;
    std::string _name;
    std::string _metatable_name;
};

}
