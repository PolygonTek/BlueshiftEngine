#pragma once

namespace LuaCpp {

namespace detail {
    template <typename T>
    struct lambda_traits : public lambda_traits<decltype(&T::operator())> {};

    template <typename T, typename Ret, typename... Args>
    struct lambda_traits<Ret(T::*)(Args...) const> {
        using Func = std::function<Ret(Args...)>;
    };
}

class Registry {
public:
    Registry(lua_State *l) : _l(l) {
        MetatableRegistry::Create(_l);
    }

    // Register C++ lambda to Lua
    template <typename L>
    void RegisterFunc(L lambda) {
        RegisterFunc((typename detail::lambda_traits<L>::Func)(lambda));
    }

    // Register C++ std::function to Lua
    template <typename Ret, typename... Args>
    void RegisterFunc(std::function<Ret(Args...)> func) {
        constexpr int arity = detail::_arity<Ret>::value;
        _funcs.emplace_back(
            std::make_unique<Func<arity, Ret, Args...>>(_l, func)
        );
    }

    // Register C++ function pointer to Lua
    template <typename Ret, typename... Args>
    void RegisterFunc(Ret(*func)(Args...)) {
        constexpr int arity = detail::_arity<Ret>::value;
        _funcs.emplace_back(
            std::make_unique<Func<arity, Ret, Args...>>(_l, func)
        );
    }

    // Register C++ object to Lua
    template <typename T>
    void RegisterObj(const std::string &name, T &obj) {
        _objs.emplace(
            std::make_pair(
                name,
                std::make_unique<Obj<T>>(_l, &obj)
            )
        );
    }

    // Find registered lua object with name
    template <typename T>
    Obj<T> *FindObj(const std::string &name) {
        auto search = _objs.find(name);
        if (search == _objs.end()) {
            return nullptr;
        }
        return static_cast<Obj<T> *>(search->second.get());
    }

    // Register C++ class to Lua
    template <typename T>
    void RegisterClass(const std::string &name) {
        _classes.emplace(
            std::make_pair(
                name,
                std::make_unique<Class<T>>(_l, name)
            )
        );
    }

    // Find registered lua class with name
    template <typename T>
    Class<T> *FindClass(const std::string &name) {
        auto search = _classes.find(name);
        if (search == _classes.end()) {
            return nullptr;
        }
        return static_cast<Class<T> *>(search->second.get());
    }    

private:
    std::vector<std::unique_ptr<BaseFunc>> _funcs;
    std::unordered_map<std::string, std::unique_ptr<BaseObj>> _objs;
    std::unordered_map<std::string, std::unique_ptr<BaseClass>> _classes;
    lua_State *_l;
};

}
