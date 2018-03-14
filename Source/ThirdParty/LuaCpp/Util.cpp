#include "LuaCpp.h"

namespace LuaCpp {

int Traceback(lua_State *l) {
    // Make nil and values not convertible to string human readable.
    const char *msg = "<not set>";
    if (!lua_isnil(l, -1)) {
        msg = lua_tostring(l, -1);
        if (!msg) {
            msg = "<error object>";
        }
    }
    lua_pushstring(l, msg);

    // call debug.traceback
    lua_getglobal(l, "debug");
    lua_getfield(l, -1, "traceback");
    lua_pushvalue(l, -3);
    lua_pushinteger(l, 2);
    lua_call(l, 2, 1);

    return 1;
}

int ErrorHandler(lua_State *l) {
    if (test_stored_exception(l) != nullptr) {
        return 1;
    }

    return Traceback(l);
}

int SetErrorHandler(lua_State *l) {
    lua_pushcfunction(l, &ErrorHandler);
    return lua_gettop(l);
}

}