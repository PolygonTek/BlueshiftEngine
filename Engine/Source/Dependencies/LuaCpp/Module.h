#pragma once

namespace LuaCpp {

class Registry;
class ExceptionHandler;

struct BaseModule {
    virtual ~BaseModule() {}
};

class Module : public BaseModule {
    friend class Selector;

public:
    template <typename T>
    Module(lua_State *l, const std::string &name, Registry &r, ExceptionHandler &eh, T callback)
        : _l(l), _name(name), _registry(&r), _exception_handler(&eh), _func([this, callback]() {
            return callback(*this);
        }) {
        lua_getglobal(l, "package");
        lua_getfield(l, -1, "preload");
        lua_pushlightuserdata(l, (void *)this);
        lua_pushcclosure(l, &_dispatcher, 1);
        lua_setfield(l, -2, name.c_str());
        lua_pop(l, 2);
    }
    ~Module() = default;
    Module(const Module &) = delete;
    Module(Module &&other) = default;
    Module &operator=(const Module &) = delete;
    Module &operator=(Module &&other) = default;

    Selector operator[](const char *name) {
        return Selector(_l, this, *_registry, *_exception_handler, name);
    }

private:
    static inline int _dispatcher(lua_State *l) {
        Module *module = (Module *)lua_touserdata(l, lua_upvalueindex(1));
        lua_newtable(l);
        module->_func();
        return 1;
    }

    lua_State *_l;
    std::string _name;
    Registry *_registry;
    ExceptionHandler *_exception_handler;
    std::function<void()> _func;
};

}