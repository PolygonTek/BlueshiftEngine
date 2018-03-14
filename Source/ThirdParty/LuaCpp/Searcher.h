#pragma once

namespace LuaCpp {

struct BaseSearcher {
    virtual ~BaseSearcher() {}
};

class Searcher : public BaseSearcher {
public:
    template <typename T>
    Searcher(lua_State *l, T callback)
        : _l(l), _func([callback](const std::string &moduleName) -> bool {
            return callback(moduleName);
        }) {
        lua_getglobal(_l, "package");                   // Push global table 'package' (1)
#if LUA_VERSION_NUM >= 502
        const char *searchers_table_name = "searchers";
#else
        const char *searchers_table_name = "loaders";
#endif
        lua_getfield(_l, -1, searchers_table_name);     // Push package.searchers field (2)
        lua_pushlightuserdata(l, (void *)this);         // Push light user data (3)
        lua_pushcclosure(l, &_dispatcher, 1);           // Push C-closure and pop associated value (3)
        // TODO: insert after first searcher
        lua_rawseti(_l, -2, 3);                         // package.searchers[3] = closure (2)
        lua_pop(_l, 2);                                 // Pop (0)
    }
    ~Searcher() = default;
    Searcher(const Searcher &) = delete;
    Searcher(Searcher &&other) = default;
    Searcher &operator=(const Searcher &) = delete;
    Searcher &operator=(Searcher &&other) = default;

private:
    static inline int _dispatcher(lua_State *l) {
        Searcher *searcher = (Searcher *)lua_touserdata(l, lua_upvalueindex(1));
        lua_pushlightuserdata(l, (void *)searcher);
        // Return loader function just for telling succeeded
        lua_pushcclosure(l, _loader, 1);
        return 1;
    }

    static inline int _loader(lua_State *l) {
#if LUA_VERSION_NUM >= 502
        const char *moduleName = lua_tostring(l, -2);
        lua_pop(l, 2);
#else
        const char *moduleName = lua_tostring(l, -1);
        lua_pop(l, 1);
#endif
        Searcher *searcher = (Searcher *)lua_touserdata(l, lua_upvalueindex(1));
        
        int top = lua_gettop(l);

        // Actually module loading will be happened here
        if (!searcher->_func(moduleName)) {
            lua_pushfstring(l, "Couldn't find '%s'", moduleName);
            return 1;
        }

        return lua_gettop(l) - top;
    }

    lua_State *_l;
    std::function<bool(const std::string &)> _func;
};

}