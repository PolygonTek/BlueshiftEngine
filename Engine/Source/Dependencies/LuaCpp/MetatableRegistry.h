#pragma once

namespace LuaCpp {

namespace detail {
    struct GetUserdataParameterFromLuaTypeError {
        std::string metatableName;
        int index;
    };
}

class MetatableRegistry {
    using TypeID = std::reference_wrapper<const std::type_info>;

public:
    // Create type info table in the registry
    static void Create(lua_State *l) {
        _create_table_in_registry(l, "luacpp_typeinfo_names");
        _create_table_in_registry(l, "luacpp_typeinfo_metatables");
    }

    // Create metatable for type
    static void PushNewMetatable(lua_State *l, TypeID type, const std::string &name);

    // Set metatable of the current object that exist on top
    static bool SetMetatable(lua_State *l, TypeID type);

    // Is registered type ?
    static bool IsRegisteredType(lua_State *l, TypeID type);

    // Get type name by TypeID
    static std::string GetTypeName(lua_State *l, TypeID type);

    // Get type name of the value at the given index
    static std::string GetTypeName(lua_State *l, int index);

    // Check type at the given index
    static bool IsType(lua_State *l, const TypeID type, int index);

    static void CheckType(lua_State *l, const TypeID type, const int index) {
        if (!IsType(l, type, index)) {
            throw LuaCpp::detail::GetUserdataParameterFromLuaTypeError{ GetTypeName(l, type), index };
        }
    }

private:
    static void _create_table_in_registry(lua_State *l, const std::string &name) {
        // push the name key
        lua_pushlstring(l, name.c_str(), name.size());
        // push the value which is new table
        lua_newtable(l);
        // registry[key] = value. pops both the key and the value from the stack
        lua_settable(l, LUA_REGISTRYINDEX);
    }

    static void _push_name_table(lua_State *l) {
        // push the key
        lua_pushliteral(l, "luacpp_typeinfo_names");
        // push the value registry[key]. pops the key from the stack
        lua_gettable(l, LUA_REGISTRYINDEX);
    }

    static void _push_metatable_table(lua_State *l) {
        // push the key
        lua_pushliteral(l, "luacpp_typeinfo_metatables");
        // push the value registry[key]. pops the key from the stack
        lua_gettable(l, LUA_REGISTRYINDEX);
    }

    static void _push_typeinfo(lua_State *l, const TypeID type) {
        // push the type_info address
        lua_pushlightuserdata(l, const_cast<std::type_info *>(&type.get()));
    }

    static void _get_name(lua_State *l, const TypeID type) {
        // push the table
        _push_name_table(l);
        // push the key
        _push_typeinfo(l, type);
        // push the value of name_table[type]. pops the key from the stack
        lua_gettable(l, -2);
        // remove name_table
        lua_remove(l, -2);
    }

    static void _get_metatable(lua_State *l, const TypeID type) {
        // push the table
        _push_metatable_table(l);
        // push the key
        _push_typeinfo(l, type);
        // push the value of metatable_table[type]. pops the key from the stack
        lua_gettable(l, -2);
        // remove metatable_table
        lua_remove(l, -2);
    }
};

}
