#pragma once

namespace LuaCpp {

namespace detail {
    struct UserData {
        enum { CreatedByCtor = 0x1 };
        intptr_t addr;
        int flags;
    };
    
    // Read a integer value from the stack
    inline int _get(_id<int>, lua_State *l, const int index) {
        return static_cast<int>(lua_tointeger(l, index));
    }
    
    // Read a unsigned integer value from the stack
    inline unsigned int _get(_id<unsigned int>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 503
        return static_cast<unsigned int>(lua_tounsigned(l, index));
#else
        return static_cast<unsigned int>(lua_tointeger(l, index));
#endif
    }

    // Read a boolean value from the stack
    inline bool _get(_id<bool>, lua_State *l, const int index) {
        return lua_toboolean(l, index) != 0;
    }
    
    // Read a integer value from the stack
    inline char _get(_id<char>, lua_State *l, const int index) {
        return static_cast<char>(lua_tointeger(l, index));
    }
    
    // Read a integer value from the stack
    inline signed char _get(_id<signed char>, lua_State *l, const int index) {
        return static_cast<signed char>(lua_tointeger(l, index));
    }
    
    // Read a integer value from the stack
    inline unsigned char _get(_id<unsigned char>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 503
        return static_cast<unsigned char>(lua_tounsigned(l, index));
#else
        return static_cast<unsigned char>(lua_tointeger(l, index));
#endif
    }

    // Read a integer value from the stack
    inline short _get(_id<short>, lua_State *l, const int index) {
        return static_cast<short>(lua_tointeger(l, index));
    }

    // Read a integer value from the stack
    inline unsigned short _get(_id<unsigned short>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 503
        return static_cast<unsigned short>(lua_tounsigned(l, index));
#else
        return static_cast<unsigned short>(lua_tointeger(l, index));
#endif
    }
    
    // Read a integer value from the stack
    inline long _get(_id<long>, lua_State *l, const int index) {
        return static_cast<long>(lua_tointeger(l, index));
    }
    
    // Read a unsigned integer value from the stack
    inline unsigned long _get(_id<unsigned long>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 503
        return static_cast<unsigned long>(lua_tounsigned(l, index));
#else
        return static_cast<unsigned long>(lua_tointeger(l, index));
#endif
    }

    // Read a integer value from the stack
    inline long long _get(_id<long long>, lua_State *l, const int index) {
        return static_cast<long long>(lua_tointeger(l, index));
    }

    // Read a unsigned integer value from the stack
    inline unsigned long long _get(_id<unsigned long long>, lua_State *l, const int index) {
#if LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM < 503
        return static_cast<unsigned long long>(lua_tounsigned(l, index));
#else
        return static_cast<unsigned long long>(lua_tointeger(l, index));
#endif
    }

    // Read a float value from the stack
    inline float _get(_id<float>, lua_State *l, const int index) {
        return (float)lua_tonumber(l, index);
    }

    // Read a double value from the stack
    inline double _get(_id<double>, lua_State *l, const int index) {
        return (double)lua_tonumber(l, index);
    }

    // Read a std::string value from the stack
    inline std::string _get(_id<std::string>, lua_State *l, const int index) {
        size_t size;
        const char *buff = lua_tolstring(l, index, &size);
        return std::string(buff, size);
    }

    // Read a std::string value from the stack
    inline const char *_get(_id<const char *>, lua_State *l, const int index) {
        return lua_tostring(l, index);
    }

    // Read a enum value from the stack
    template <typename T>
    inline std::enable_if_t<std::is_enum<T>::value, T> _get(_id<T>, lua_State *l, const int index) {
        return _get(_id<std::underlying_type_t<T>>(), l, index);
    }

    // Read a primitive type pointer from the stack
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value, T> *_get(_id<T*>, lua_State *l, const int index) {
        return (T *)lua_topointer(l, index);
    }

    // Read a primitive type refernece from the stack
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value, T> &_get(_id<T&>, lua_State *l, const int index) {
        return *(T *)lua_topointer(l, index);
    }

    // Read a user data pointer from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> *_get(_id<T*>, lua_State *l, const int index) {
        if (MetatableRegistry::IsType(l, typeid(T), index)) {
            return *(T **)lua_topointer(l, index);
        }
        return nullptr;
    }

    // Read a user data refernece from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> &_get(_id<T&>, lua_State *l, const int index) {
        if (!MetatableRegistry::IsType(l, typeid(T), index)) {
            throw TypeError{ MetatableRegistry::GetTypeName(l, typeid(T)), MetatableRegistry::GetTypeName(l, index) };
        }

        T *ptr = *(T **)lua_topointer(l, index);
        if (ptr == nullptr) {
            throw TypeError{ MetatableRegistry::GetTypeName(l, typeid(T)) };
        }
        return *ptr;
    }

    // Read a user data value from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> _get(_id<T>, lua_State *l, const int index) {
        return _get(_id<T&>{}, l, index);
    }

    using _lua_check_get = void(*)(lua_State *l, int index);

    // Throw this on conversion errors to prevent long jumps caused in Lua from
    // bypassing destructors. The outermost function can then call checkd_get(index)
    // in a context where a long jump is safe.
    // This way we let Lua generate the error message and use proper stack unwinding.
    struct GetParameterFromLuaTypeError {
        _lua_check_get checked_get;
        int index;
    };
    
    // Check read a integer value from the stack
    inline int _check_get(_id<int>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 502
        auto res = static_cast<int>(lua_tointegerx(l, index, &isNum));
#else
        auto res = static_cast<int32_t>(lua_tointeger(l, index));
        isNum = (res != 0 || lua_isnumber(l, index));
#endif
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
#if LUA_VERSION_NUM >= 503
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
#else
                [](lua_State *l, int index) { luaL_checkint(l, index); },
#endif
                index
            };
        }
        return res;
    }
    
    // Check read a unsigned integer value from the stack
    inline unsigned int _check_get(_id<unsigned int>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 503
        auto res = static_cast<unsigned int>(lua_tointegerx(l, index, &isNum));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
                index
            };
        }
#elif LUA_VERSION_NUM >= 502
        auto res = static_cast<unsigned int>(lua_tounsignedx(l, index, &isNum));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkunsigned(l, index); },
                index
            };
        }
#else
        auto res = static_cast<unsigned int>(lua_tointeger(l, index));
        isNum = (res != 0 || lua_isnumber(l, index));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
                index
            };
        }
#endif
        return res;
    }

    // Check read a boolean value from the stack
    inline bool _check_get(_id<bool>, lua_State *l, const int index) {
        return lua_toboolean(l, index) != 0;
    }
    
    // Check read a integer value from the stack
    inline char _check_get(_id<char>, lua_State *l, const int index) {
        return (char)_check_get(_id<int>(), l, index);
    }
    
    // Check read a integer value from the stack
    inline signed char _check_get(_id<signed char>, lua_State *l, const int index) {
        return (signed char)_check_get(_id<int>(), l, index);
    }
    
    // Check read a unsigned integer value from the stack
    inline unsigned char _check_get(_id<unsigned char>, lua_State *l, const int index) {
        return (unsigned char)_check_get(_id<unsigned int>(), l, index);
    }
    
    // Check read a integer value from the stack
    inline short _check_get(_id<short>, lua_State *l, const int index) {
        return (short)_check_get(_id<int>(), l, index);
    }
    
    // Check read a unsigned integer value from the stack
    inline unsigned short _check_get(_id<unsigned short>, lua_State *l, const int index) {
        return (unsigned short)_check_get(_id<unsigned int>(), l, index);
    }
    
    // Check read a integer value from the stack
    inline long _check_get(_id<long>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 502
        auto res = static_cast<long>(lua_tointegerx(l, index, &isNum));
#else
        auto res = static_cast<long>(lua_tointeger(l, index));
        isNum = (res != 0 || lua_isnumber(l, index));
#endif
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
#if LUA_VERSION_NUM >= 503
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
#else
                [](lua_State *l, int index) { luaL_checkint(l, index); },
#endif
                index
            };
        }
        return res;
    }
    
    // Check read a unsigned integer value from the stack
    inline unsigned long _check_get(_id<unsigned long>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 503
        auto res = static_cast<uint64_t>(lua_tointegerx(l, index, &isNum));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
                index
            };
        }
#elif LUA_VERSION_NUM >= 502
        auto res = static_cast<unsigned long>(lua_tounsignedx(l, index, &isNum));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkunsigned(l, index); },
                index
            };
        }
#else
        auto res = static_cast<unsigned long>(lua_tointeger(l, index));
        isNum = (res != 0 || lua_isnumber(l, index));
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkinteger(l, index); },
                index
            };
        }
#endif
        return res;
    }

    // Check read a integer value from the stack
    inline long long _check_get(_id<long long>, lua_State *l, const int index) {
        return _check_get(_id<long>(), l, index);
    }

    // Check read a unsigned integer value from the stack
    inline unsigned long long _check_get(_id<unsigned long long>, lua_State *l, const int index) {
        return _check_get(_id<unsigned long>(), l, index);
    }
    
    // Check read a lua_Number value from the stack
    inline float _check_get(_id<float>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 502
        auto res = lua_tonumberx(l, index, &isNum);
#else
        auto res = lua_tonumber(l, index);
        isNum = (res != 0 || lua_isnumber(l, index));
#endif
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checknumber(l, index); }, index
            };
        }
        return (float)res;
    }

    // Check read a lua_Number value from the stack
    inline double _check_get(_id<double>, lua_State *l, const int index) {
        int isNum = 0;
#if LUA_VERSION_NUM >= 502
        auto res = lua_tonumberx(l, index, &isNum);
#else
        auto res = lua_tonumber(l, index);
        isNum = (res != 0 || lua_isnumber(l, index));
#endif        
        if (!isNum) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checknumber(l, index); }, index
            };
        }
        return res;
    }    

    // Check read a std::string value from the stack
    inline std::string _check_get(_id<std::string>, lua_State *l, const int index) {
        size_t size = 0;
        const char *buff = lua_tolstring(l, index, &size);
        if (buff == nullptr) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkstring(l, index); },
                index
            };
        }
        return std::string(buff, size);
    }

    // Check read a cstring from the stack
    inline const char *_check_get(_id<const char *>, lua_State *l, const int index) {
        const char *buff = lua_tostring(l, index);
        if (buff == nullptr) {
            throw GetParameterFromLuaTypeError{
                [](lua_State *l, int index) { luaL_checkstring(l, index); },
                index
            };
        }
        return buff;
    }

    // Check read a enum value from the stack
    template <typename T>
    inline std::enable_if_t<std::is_enum<T>::value, T> _check_get(_id<T>, lua_State *l, const int index) {
        return (T)_check_get(_id<std::underlying_type_t<T>>{}, l, index);
    }

    // Check read a primitive type pointer from the stack
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value, T> *_check_get(_id<T*>, lua_State *l, const int index) {
        return (T *)lua_topointer(l, index);
    }

    // Check read a primitive type reference from the stack
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value, T> &_check_get(_id<T&>, lua_State *l, const int index) {
        return *(T *)lua_topointer(l, index);
    }

    // Check read a user data pointer from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> *_check_get(_id<T*>, lua_State *l, const int index) {
        MetatableRegistry::CheckType(l, typeid(T), index);
        return *(T **)lua_topointer(l, index);
    }

    // Check read a user data reference from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> &_check_get(_id<T&>, lua_State *l, const int index) {
        static_assert(!is_primitive<T>::value, "Reference types must not be primitives.");

        T *ptr = _check_get(_id<T*>{}, l, index);
        if (ptr == nullptr) {
            throw GetUserdataParameterFromLuaTypeError{ MetatableRegistry::GetTypeName(l, typeid(T)), index };
        }

        return *ptr;
    }

    // Check read a user data value from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> _check_get(_id<T>, lua_State *l, const int index) {
        return _check_get(_id<T&>{}, l, index);
    };

    // Check read a user data by rvalue reference from the stack
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value, T> _check_get(_id<T&&>, lua_State *l, const int index) {
        return _check_get(_id<T>{}, l, index);
    }

    // Worker type-trait struct to _get_n
    // Getting multiple elements returns a tuple
    template <typename... Ts>
    struct _get_n_impl {
        using type = std::tuple<Ts...>;

        template <std::size_t... N>
        static type worker(lua_State *l, _indices<N...>) {
            return std::make_tuple(_get(_id<Ts>{}, l, N + 1)...);
        }

        static type apply(lua_State *l) {
            return worker(l, typename _indices_builder<sizeof...(Ts)>::type());
        }
    };

    // Getting nothing returns void
    template <>
    struct _get_n_impl<> {
        using type = void;
        static type apply(lua_State *) {}
    };

    // Getting one element returns an unboxed value
    template <typename T>
    struct _get_n_impl<T> {
        using type = T;
        static type apply(lua_State *l) {
            return _get(_id<T>{}, l, -1);
        }
    };

    template <typename... T>
    typename _get_n_impl<T...>::type _get_n(lua_State *l) {
        return _get_n_impl<T...>::apply(l);
    }

    template <typename T>
    T _pop(_id<T> t, lua_State *l) {
        T ret = _get(t, l, -1);
        lua_pop(l, 1);
        return ret;
    }

    inline void _push(lua_State *) {}

    inline void _push(lua_State *l, bool b) {
        lua_pushboolean(l, b);
    }

    inline void _push(lua_State *l, int i) {
        lua_pushinteger(l, i);
    }    

    inline void _push(lua_State *l, unsigned int u) {
#if LUA_VERSION_NUM >= 503
        lua_pushinteger(l, (lua_Integer)u);
#elif LUA_VERSION_NUM >= 502
        lua_pushunsigned(l, u);
#else
        lua_pushinteger(l, static_cast<int>(u));
#endif
    }

    inline void _push(lua_State *l, float f) {
        lua_pushnumber(l, (lua_Number)f);
    }

    inline void _push(lua_State *l, double f) {
        lua_pushnumber(l, (lua_Number)f);
    }

    inline void _push(lua_State *l, const std::string &s) {
        lua_pushlstring(l, s.c_str(), s.size());
    }

    inline void _push(lua_State *l, const char *s) {
        lua_pushstring(l, s);
    }

    // Push a enum value
    template <typename T>
    inline std::enable_if_t<std::is_enum<T>::value> _push(lua_State *l, T t) {
        return _push(l, (int)t);
    }

    // Push a primitive or enum type pointer
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value> _push(lua_State *l, T *t) {
        if (t == nullptr) {
            // or just nil
            lua_pushnil(l);
        } else {
            lua_pushlightuserdata(l, (void *)&t);
        }
    }

    // Push a primitive type reference
    template <typename T>
    inline std::enable_if_t<is_primitive_decay<T>::value && !std::is_enum<T>::value> _push(lua_State *l, T &&t) {
        lua_pushlightuserdata(l, (void *)&t);
    }

    // Push a userdata pointer
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value> _push(lua_State *l, T *t) {
        if (t == nullptr) {
            // or just nil
            lua_pushnil(l);
        } else {
            UserData *userdata = (UserData *)lua_newuserdata(l, sizeof(UserData));
            userdata->flags = 0;
            userdata->addr = (intptr_t)t;
            MetatableRegistry::SetMetatable(l, typeid(T));
        }
    }

    // Push a userdata lvalue reference
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value> _push(lua_State *l, T &t) {
        UserData *userdata = (UserData *)lua_newuserdata(l, sizeof(UserData));
        userdata->flags = 0;
        userdata->addr = (intptr_t)&t;
        MetatableRegistry::SetMetatable(l, typeid(T));
    }

    // Push a userdata rvalue reference
    template <typename T>
    inline std::enable_if_t<!is_primitive_decay<T>::value && std::is_rvalue_reference<T&&>::value> _push(lua_State *l, T &&t) {
        if (!MetatableRegistry::IsRegisteredType(l, typeid(t))) {
            throw CopyUnregisteredType(typeid(t));
        }
        UserData *userdata = (UserData *)lua_newuserdata(l, sizeof(UserData) + sizeof(T));
        void *data = (void *)&userdata[1];
        userdata->flags = UserData::CreatedByCtor;
        userdata->addr = (intptr_t)data;
        new (data) T(std::forward<T>(t));
        MetatableRegistry::SetMetatable(l, typeid(T));
    }

    inline void _push_n(lua_State *) {}

    template <typename T, typename... Rest>
    inline void _push_n(lua_State *l, T &&value, Rest&&... rest) {
        _push(l, std::forward<T>(value));
        _push_n(l, std::forward<Rest>(rest)...);
    }

    template <typename... T, std::size_t... N>
    inline void _push_dispatcher(lua_State *l, const std::tuple<T...> &values, std::index_sequence<N...>) {
        _push_n(l, std::get<N>(values)...);
    }

    inline void _push(lua_State *, std::tuple<>) {}

    template <typename... T>
    inline void _push(lua_State *l, const std::tuple<T...> &values) {
        constexpr int num_values = sizeof...(T);
        _push_dispatcher(l, values, typename std::make_index_sequence<num_values>::type());
    }

    template <typename... T>
    inline void _push(lua_State *l, std::tuple<T...> &&values) {
        _push(l, const_cast<const std::tuple<T...> &>(values));
    }
}

}
