#include "LuaCpp.h"

namespace LuaCpp {

void State::RegisterLib(const struct luaL_Reg *lib, const char *libname) {
    lua_getglobal(_l, "_G");
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(_l, lib, 0);
#else
    luaL_register(_l, libname, lib);
#endif
    lua_pop(_l, 1);
}

void State::Require(const char *name) {
    lua_getglobal(_l, "require");
    lua_pushstring(_l, name);
    lua_call(_l, 1, 1);
}

void State::Require(const char *name, lua_CFunction openf) {
#if LUA_VERSION_NUM >= 502
    luaL_requiref(_l, name, openf, 1);
#else
    luaL_checkstack(_l, 3, "not enough stack slots");
    lua_pushcfunction(_l, openf);
    lua_pushstring(_l, name);
    lua_call(_l, 1, 1);
    lua_getglobal(_l, "package");
    lua_getfield(_l, -1, "loaded");
    lua_replace(_l, -2);
    lua_pushvalue(_l, -2);
    lua_setfield(_l, -2, name);
    lua_pop(_l, 1);
    lua_pushvalue(_l, -1);
    lua_setglobal(_l, name);
#endif
    lua_pop(_l, 1);  /* remove lib */
}

void State::SetToNil(const char *name) {
    ResetStackOnScopeExit savedStack(_l);
    lua_getglobal(_l, name);
    if (lua_type(_l, -1) != LUA_TNIL) {
        lua_pop(_l, 1);
        lua_pushnil(_l);
        lua_setglobal(_l, name);
    }
}

bool State::LoadBuffer(const std::string &name, const char *code, size_t size, const char *sandbox) {
    if (size == 0) {
        size = strlen(code);
    }

#if 0
    // Remove path
    size_t pos = name.find_last_of("/\\");
    if (pos == std::string::npos) {
        _name = name;
    } else {
        _name = name.substr(pos + 1);
    }
#else
    _name = name;
#endif
    // lua_load pushes the compiled chunk as a Lua function on top of the stack. 
    // Otherwise, it pushes an error message.
    int status = luaL_loadbuffer(_l, code, size, _name.c_str());
    if (status != 0) {
        if (status == LUA_ERRSYNTAX) {
            const char *msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : _name + ": syntax error");
        } else if (status == LUA_ERRMEM) {
            const char *msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : _name + ": out-of-memory error");
        }
        return false;
    }

    // Assume that function already pushed on top
    assert(lua_type(_l, -1) == LUA_TFUNCTION);

    if (sandbox && sandbox[0]) {
        lua_newtable(_l);                   // Push new environment table for sandboxing (1)
        lua_setglobal(_l, sandbox);         // Set and pop (0)
        lua_getglobal(_l, sandbox);         // Push again (1)
        lua_newtable(_l);                   // Push new metatable (2)
        lua_getglobal(_l, "_G");            // Push global table (3)
        lua_setfield(_l, -2, "__index");    // Do metatable[__index] = _G and pop (2)
        lua_setmetatable(_l, -2);           // Set metatable of sandbox table and pop (1)
#if LUA_VERSION_NUM >= 502
        lua_setupvalue(_l, -2, 1);          // Pop and set sandbox table as the new environment (using upvalue) of the current function (0)
#else
        lua_setfenv(_l, -2);                // Pop and set sandbox table as the new environment of the current function (0)
#endif
    }

    return true;
}

bool State::Run() {
    int status = lua_pcall(_l, 0, LUA_MULTRET, 0);
    if (status == 0) {
        return true;
    }

    const char *msg = lua_tostring(_l, -1);
    _exception_handler->Handle(status, msg ? msg : std::string("Failed to run ") + _name);
    return false;
}

bool State::RunBuffer(const std::string &name, const char *code, size_t size, const char *sandbox) {
    if (!LoadBuffer(name, code, size, sandbox)) {
        return false;
    }   

    return Run();
}

bool State::Compile(const std::string &name, const char *text, std::string &byteCode) const {
    ResetStackOnScopeExit savedStack(_l);
    int status = luaL_loadstring(_l, text);
    if (status != 0) {
        if (status == LUA_ERRSYNTAX) {
            const char *msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : name + ": syntax error");
        } else if (status == LUA_ERRMEM) {
            const char *msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : name + ": out-of-memory error");
        }
        return false;
    }

    luaL_Buffer buff;
    luaL_buffinit(_l, &buff);
#if LUA_VERSION_NUM >= 503
    status = lua_dump(_l, _ldump_writer, &buff, true);
#else
    status = lua_dump(_l, _ldump_writer, &buff);
#endif
    if (status != 0) {
        return false;
    }

    luaL_pushresult(&buff);
    size_t size;
    const char *code = lua_tolstring(_l, -1, &size);
    byteCode = std::string(code, size);
    return true;
}

bool State::operator()(const char *code) {
    ResetStackOnScopeExit savedStack(_l);
    int status = luaL_loadstring(_l, code);
    if (status != 0) {
        _exception_handler->Handle_top_of_stack(status, _l);
        return false;
    }

    status = lua_pcall(_l, 0, LUA_MULTRET, 0);
    if (status != 0) {
        _exception_handler->Handle_top_of_stack(status, _l);
        return false;
    }
    return true;
}

}
