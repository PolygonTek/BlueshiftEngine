#pragma once

namespace LuaCpp {

class Selector;

namespace detail {
    struct function_base {
        function_base(int ref, lua_State *l)
            : _ref(l, ref), _l(l), _exception_handler(nullptr) {}

        void _enable_exception_handler(ExceptionHandler *exception_handler) {
            _exception_handler = exception_handler;
        }

        void protected_call(int const num_args, int const num_ret, int const handler_index) {
            const auto status = lua_pcall(_l, num_args, num_ret, handler_index);

            if (status != 0 && _exception_handler) {
                _exception_handler->Handle_top_of_stack(status, _l);
            }
        }

        void Push(lua_State *l) const {
            _ref.Push(l);
        }

        LuaRef _ref;
        lua_State *_l;
        ExceptionHandler *_exception_handler;
    };
}

// Similar to an std::function but refers to a lua function
template <class>
class function {};

template <typename Ret, typename... Args>
class function<Ret(Args...)> : detail::function_base {
    friend class Selector;

public:
    using function_base::function_base;

    Ret operator()(Args... args) {
        ResetStackOnScopeExit save(_l);

        int handler_index = SetErrorHandler(_l);
        _ref.Push(_l);
        detail::_push_n(_l, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);

        protected_call(num_args, 1, handler_index);

        return detail::_get(detail::_id<Ret>{}, _l, -1);
    }

    using function_base::Push;
};

template <typename... Args>
class function<void(Args...)> : detail::function_base {
    friend class Selector;

public:
    using function_base::function_base;

    void operator()(Args... args) {
        ResetStackOnScopeExit save(_l);

        int handler_index = SetErrorHandler(_l);
        _ref.Push(_l);
        detail::_push_n(_l, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);

        protected_call(num_args, 1, handler_index);
    }

    using function_base::Push;
};

// Specialization for multireturn types
template <typename... R, typename... Args>
class function<std::tuple<R...>(Args...)> : detail::function_base {
    friend class Selector;

public:
    using function_base::function_base;

    std::tuple<R...> operator()(Args... args) {
        ResetStackOnScopeExit save(_l);

        int handler_index = SetErrorHandler(_l);
        _ref.Push(_l);
        detail::_push_n(_l, std::forward<Args>(args)...);
        constexpr int num_args = sizeof...(Args);
        constexpr int num_ret = sizeof...(R);

        protected_call(num_args, num_ret, handler_index);

        lua_remove(_l, handler_index);
        return detail::_get_n<R...>(_l);
    }

    using function_base::Push;
};

namespace detail {
    template <typename T>
    struct is_primitive<LuaCpp::function<T>> {
        static constexpr bool value = true;
    };

    template <typename Ret, typename... Args>
    inline LuaCpp::function<Ret(Args...)> _check_get(_id<LuaCpp::function<Ret(Args...)>>, lua_State *l, const int index) {
        lua_pushvalue(l, index);
        return LuaCpp::function<Ret(Args...)>{ luaL_ref(l, LUA_REGISTRYINDEX), l };
    }

    template <typename Ret, typename... Args>
    inline LuaCpp::function<Ret(Args...)> _get(_id<LuaCpp::function<Ret(Args...)>> id, lua_State *l, const int index) {
        return _check_get(id, l, index);
    }

    template <typename Ret, typename... Args>
    inline void _push(lua_State *l, LuaCpp::function<Ret(Args...)> func) {
        func.Push(l);
    }
}

}
