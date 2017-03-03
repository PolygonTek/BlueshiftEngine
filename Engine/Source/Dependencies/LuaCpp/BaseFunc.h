#pragma once

namespace LuaCpp {

struct BaseFunc {
    virtual ~BaseFunc() {}
    virtual int Invoke(lua_State *l) = 0;
    virtual BaseFunc *Clone(lua_State *l, const std::string &metatableName) { return nullptr; }
};

namespace detail {
    int _lua_dispatcher(lua_State *l);

    // Read values from the stack and return packed tuple
    template <typename... T>
    inline std::tuple<T...> _get_args(lua_State *l) {
        constexpr std::size_t num_args = sizeof...(T);
        // make_index_sequence<N>() will be replace by std::index_sequence<0, 1, 2, 3, ..., N-1>()
        return _get_args<T...>(l, typename _indices_builder<num_args>::type());
    }

    template <typename... T, std::size_t... N>
    inline std::tuple<T...> _get_args(lua_State *l, _indices<N...>) {
        return std::tuple<T...>(_check_get(_id<T>{}, l, N + 1)...);
    }    

    // Call function with arguments in tuple
    template <typename Ret, typename... Args>
    inline Ret _lift(std::function<Ret(Args...)> func, std::tuple<Args...> args) {
        // make_index_sequence<N>() will be replace by std::index_sequence<0, 1, 2, 3, ..., N-1>()
        return _lift(func, args, typename _indices_builder<sizeof...(Args)>::type());
    }

    template <typename Ret, typename... Args, std::size_t... N>
    inline Ret _lift(std::function<Ret(Args...)> func, std::tuple<Args...> args, _indices<N...>) {
        // Call function with arguments from unpacked tuple
        return func(std::get<N>(args)...);
    }    
}

}
