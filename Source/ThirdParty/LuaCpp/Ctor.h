#pragma once

namespace LuaCpp {

template <typename T, typename... Args>
class Ctor : public BaseFunc {
    using _ctor_type = std::function<void(lua_State*, Args...)>;

public:    
    Ctor(lua_State *l, const std::string &name, const std::string &metatableName) 
        : _name(name), _ctor([metatableName](lua_State *l, Args... args) {
            // Create user data with sizeof T
            detail::UserData *userdata = (detail::UserData *)lua_newuserdata(l, sizeof(detail::UserData) + sizeof(T));
            void *data = (void *)&userdata[1];
            userdata->flags = detail::UserData::CreatedByCtor;
            userdata->addr = (intptr_t)data;
            // Call ctor of it
            new (data) T(args...);
            // Set it's metatable
#if LUA_VERSION_NUM >= 502
            luaL_setmetatable(l, metatableName.c_str());
#else
            luaL_checkstack(l, 1, "not enough stack slots");
            luaL_getmetatable(l, metatableName.c_str());
            lua_setmetatable(l, -2);
#endif
        }) {
        // NOTE: Assume that metatable of class is already pushed on top
        lua_pushlightuserdata(l, (void *)static_cast<BaseFunc *>(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);        
        lua_setfield(l, -2, name.c_str());
        _is_table_call = _name == "__call" ? true : false;
    }

    virtual int Invoke(lua_State *l) override {
        // First argument of __call is a table itself so we need to strip out
        if (_is_table_call) {
            lua_remove(l, -(int)(sizeof...(Args) + 1));
        }
        // Read all arguments from the stack and pack it into a tuple
        std::tuple<Args...> args = detail::_get_args<Args...>(l);
        auto pack = std::tuple_cat(std::make_tuple(l), args);
        // Call the function '_ctor' with those arguments by converting the tuple back into a parameter pack.
        detail::_lift(_ctor, pack);
        // The constructor will leave a single userdata entry on the stack
        return 1;
    }

private:
    _ctor_type _ctor;
    std::string _name;
    bool _is_table_call;
};

}
