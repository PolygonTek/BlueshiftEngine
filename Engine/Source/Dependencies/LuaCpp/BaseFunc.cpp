#include "LuaCpp.h"

namespace LuaCpp {

namespace detail {
    int _lua_dispatcher(lua_State *l) {
        BaseFunc *func = (BaseFunc *)lua_touserdata(l, lua_upvalueindex(1));

        _lua_check_get raiseParameterConversionError = nullptr;
        const char *wrongMetaTableName = nullptr;
        int erroneousParameterIndex = 0;

        try {
            return func->Invoke(l);
        } catch (GetParameterFromLuaTypeError &e) {
            raiseParameterConversionError = e.checked_get;
            erroneousParameterIndex = e.index;
        } catch (GetUserdataParameterFromLuaTypeError &e) {
#if LUA_VERSION_NUM >= 502
            wrongMetaTableName = lua_pushlstring(l, e.metatableName.c_str(), e.metatableName.length());
#else
            wrongMetaTableName = lua_pushfstring(l, e.metatableName.c_str());
#endif
            erroneousParameterIndex = e.index;
        } catch (std::exception &e) {
            lua_pushstring(l, e.what());
            Traceback(l);
            store_current_exception(l, lua_tostring(l, -1));
        } catch (...) {
            lua_pushliteral(l, "<Unknown exception>");
            Traceback(l);
            store_current_exception(l, lua_tostring(l, -1));
        }

        if (raiseParameterConversionError) {
            raiseParameterConversionError(l, erroneousParameterIndex);
        } else if (wrongMetaTableName) {
            luaL_checkudata(l, erroneousParameterIndex, wrongMetaTableName);
        }

        return lua_error(l);
    }
}

}