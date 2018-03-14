#pragma once

namespace LuaCpp {

template <typename T>
class Dtor : public BaseFunc {
public:
    Dtor(lua_State *l, const std::string &metatableName)
        : _metatable_name(metatableName) {
        // NOTE: Assume that metatable of class is already pushed on top
        lua_pushlightuserdata(l, (void *)(this));
        lua_pushcclosure(l, &detail::_lua_dispatcher, 1);
        // When userdata is set to be garbage collected, if the metatable has a __gc field pointing to a function, 
        // that function is first invoked, passing the userdata to it
        lua_setfield(l, -2, "__gc");
    }

    virtual int Invoke(lua_State *l) override {
#if LUA_VERSION_NUM >= 502
        // Lua 5.2 supports the __gc metamethod for tables.
        if (lua_type(l, 1) != LUA_TUSERDATA) {
            return 0;
        }
#endif
        detail::UserData *userdata = (detail::UserData *)luaL_checkudata(l, 1, _metatable_name.c_str());
        if (userdata->flags & detail::UserData::CreatedByCtor) {
            T *self = (T *)userdata->addr;
            // Call dtor
            self->~T();
        }
        // There are no return values
        return 0;
    }

private:
    std::string _metatable_name;
};

}
