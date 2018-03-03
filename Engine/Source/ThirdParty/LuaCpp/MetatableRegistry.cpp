#include "LuaCpp.h"

namespace LuaCpp {

void MetatableRegistry::PushNewMetatable(lua_State *l, TypeID type, const std::string &name) {
    // push the name_table
    _push_name_table(l);
    // push the key
    _push_typeinfo(l, type);
    // push the name
    lua_pushlstring(l, name.c_str(), name.size());
    // names_table[type] = name
    lua_settable(l, -3);
    // pops the names_table
    lua_pop(l, 1);

    // push the new metatable which is exist in the registry
    luaL_newmetatable(l, name.c_str()); // Actual result.

                                        // push the metatable_table
    _push_metatable_table(l);
    // push the key
    _push_typeinfo(l, type);
    // push the metatable
    lua_pushvalue(l, -3);
    // metatable_table[type] = metatable
    lua_settable(l, -3);
    // pops the metatable_table
    lua_pop(l, 1);
}

bool MetatableRegistry::SetMetatable(lua_State *l, TypeID type) {
    // get metatable_table[type] and push it
    _get_metatable(l, type);

    // if type has a valid metatable
    if (lua_istable(l, -1)) {
        // set metatable
        lua_setmetatable(l, -2);
        return true;
    }

    lua_pop(l, 1);
    return false;
}

bool MetatableRegistry::IsRegisteredType(lua_State *l, TypeID type) {
    // get name_table[type] and push it
    _get_name(l, type);

    bool registered = !!lua_isstring(l, -1);
    lua_pop(l, 1);
    return registered;
}

// Get type name by TypeID
std::string MetatableRegistry::GetTypeName(lua_State *l, TypeID type) {
    std::string name("unregistered type");

    // get name_table[type] and push it
    _get_name(l, type);

    if (lua_isstring(l, -1)) {
        size_t len = 0;
        char const *str = lua_tolstring(l, -1, &len);
        name.assign(str, len);
    }

    lua_pop(l, 1);
    return name;
}

// Get type name of the value at the given index
std::string MetatableRegistry::GetTypeName(lua_State *l, int index) {
    std::string name;

    if (lua_getmetatable(l, index)) {
        lua_pushliteral(l, "__name");
        lua_gettable(l, -2);

        if (lua_isstring(l, -1)) {
            size_t len = 0;
            char const * str = lua_tolstring(l, -1, &len);
            name.assign(str, len);
        }

        lua_pop(l, 2);
    }

    if (name.empty()) {
        name = lua_typename(l, lua_type(l, index));
    }

    return name;
}

// Check type at the given index
bool MetatableRegistry::IsType(lua_State *l, const TypeID type, int index) {
    bool equal = true;

    if (lua_getmetatable(l, index)) {
        _get_metatable(l, type);
        equal = lua_istable(l, -1) && lua_rawequal(l, -1, -2);
        lua_pop(l, 2);
    } else {
        _get_metatable(l, type);
        equal = !lua_istable(l, -1);
        lua_pop(l, 1);
    }

    return equal;
}

}