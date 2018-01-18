#pragma once

#ifdef HAS_REF_QUALIFIERS
    #undef HAS_REF_QUALIFIERS
    #undef REF_QUAL_LVALUE
#endif

#if defined(__clang__)
    #if __has_feature(cxx_reference_qualified_functions)
        #define HAS_REF_QUALIFIERS
    #endif
#elif defined(__GNUC__)
    #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #if GCC_VERSION >= 40801
        #define HAS_REF_QUALIFIERS
    #endif
#elif defined(_MSC_VER)
    #if _MSC_VER >= 1900 // since MSVS-14 CTP1
        #define HAS_REF_QUALIFIERS
    #endif
#endif

#if defined(HAS_REF_QUALIFIERS)
    #define REF_QUAL_LVALUE &
#else
    #define REF_QUAL_LVALUE
#endif

namespace LuaCpp {

class State;
class Module;

class Selector {
    friend class State;
    friend class Module;

public:
    Selector() : _l(nullptr) {}
    Selector(const Selector &) = default;
    Selector(Selector &&) = default;
    Selector &operator=(const Selector &) = default;
    Selector &operator=(Selector &&) = default;

    ~Selector() {
        // If there is a functor is not empty, execute it and collect no args
        if (_functor_active) {
            ResetStackOnScopeExit save(_l);
            _traverse_full();

            if (std::uncaught_exception()) {
                try {
                    _call_functor(0);
                } catch (...) {
                    // We are already unwinding, ignore further exceptions.
                    // As of C++17 consider std::uncaught_exceptions()
                }
            } else {
                _call_functor(0);
            }
        }
    }

    // Allow automatic casting when used in comparisons
    bool operator==(Selector &other) = delete;

    // Compare a retrieved value that is returned by call functor with a string
    friend bool operator==(const Selector &, const char *);
    friend bool operator==(const char *, const Selector &);

    bool IsValid() const { return _l != nullptr; }
    const std::string Name() const { return _name; }
    const std::string LastName() const {
        size_t pos = _name.find_last_of('.');
        if (pos == std::string::npos) {
            return _name;
        }
        return _name.substr(pos + 1);
    }

    // Prepare to call functor 'copy' that will be called later
    template <typename... Args>
    const Selector operator()(Args&&... args) const {
        Selector copy(*this);
        const auto state = _l; // gcc-5.1 doesn't support implicit member capturing
        const auto eh = _exception_handler;
        copy._functor_arguments = make_Refs(_l, std::forward<Args>(args)...);
        copy._functor_active = true;
        return copy;
    }

    // Set the element to a boolean value
    void operator=(bool b) const {
        _evaluate_store([this, b]() {
            detail::_push(_l, b);
        });
    }

    // Set the element to a char value
    void operator=(char c) const {
        _evaluate_store([this, c]() {
            detail::_push(_l, (unsigned int)c);
        });
    }

    // Set the element to a int8_t value
    void operator=(int8_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (int)i);
        });
    }

    // Set the element to a uint8_t value
    void operator=(uint8_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (unsigned int)i);
        });
    }

    // Set the element to a int16_t value
    void operator=(int16_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (int)i);
        });
    }

    // Set the element to a uint16_t value
    void operator=(uint16_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (unsigned int)i);
        });
    }

    // Set the element to a int32_t value
    void operator=(int32_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (int)i);
        });
    }

    // Set the element to a uint32_t value
    void operator=(uint32_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, (unsigned int)i);
        });
    }

    // Set the element to a int64_t value
    void operator=(int64_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, i);
        });
    }

    // Set the element to a uint64_t value
    void operator=(uint64_t i) const {
        _evaluate_store([this, i]() {
            detail::_push(_l, i);
        });
    }

    // Set the element to a float value
    void operator=(float f) const {
        _evaluate_store([this, f]() {
            detail::_push(_l, f);
        });
    }

    // Set the element to a double value
    void operator=(double f) const {
        _evaluate_store([this, f]() {
            detail::_push(_l, f);
        });
    }

    // Set the element to a string
    void operator=(const std::string &s) const {
        _evaluate_store([this, s]() {
            detail::_push(_l, s);
        });
    }

    // Set the element to a string
    void operator=(const char *s) const {
        _evaluate_store([this, s]() {
            detail::_push(_l, s);
        });
    }

    // Set the element to a reference value
    template <typename T, typename = std::enable_if_t<!std::is_enum<T>::value>>
    void operator=(T const &ref) {
        _evaluate_store([this, &ref]() {
            detail::_push(_l, ref);
        });
    }

    // Set the element to a Reference value
    template <typename T>
    void operator=(Reference<T> const &ref) {
        _evaluate_store([this, &ref]() {
            detail::_push(_l, ref);
        });
    }

    // Set the element to a Pointer value
    template <typename T>
    void operator=(Pointer<T> const &ptr) {
        _evaluate_store([this, &ptr]() {
            detail::_push(_l, ptr);
        });
    }    

    // Set the element to a function pointer
    template <typename Ret, typename... Args>
    void SetFunc(Ret(*func)(Args...)) {
        _evaluate_store([this, func]() {
            _registry->RegisterFunc(func);
        });
    }

    // Set the element to a std::function
    template <typename Ret, typename... Args>
    void SetFunc(std::function<Ret(Args...)> func) {
        _evaluate_store([this, func]() {
            _registry->RegisterFunc(func);
        });
    }
    
    // Set the element to a lambda
    template <typename L, typename = std::enable_if_t<detail::is_callable<L>::value>>
    void SetFunc(L lambda) const {
        _evaluate_store([this, lambda]() {
            _registry->RegisterFunc(lambda);
        });
    }

    // Set the element to C++ object table
    template <typename T>
    void SetObj(T &object) {
        std::string fullname = _fullname();
        _evaluate_store([this, &fullname, &object]() {
            _registry->RegisterObj(fullname, object);
        });
    }

    // Add object members
    template <typename T, typename... Members>
    void AddObjMembers(T &object, Members... members) {
        auto *luaObj = _registry->FindObj<T>(_fullname());
        if (luaObj) {
            ResetStackOnScopeExit save(_l);
            _traverse_full();
            // TODO: handle exception when this element is not an Obj
            luaObj->RegisterMembers(_l, &object, members...);
        }
    }

    // Set the element to C++ class table
    template <typename T>
    void SetClass() {
        std::string fullname = _fullname();
        _evaluate_store([this, &fullname]() {
            _registry->RegisterClass<T>(fullname);
        });
    }

    // Set the element to C++ class table and inherit member functions from parent
    template <typename T>
    void SetClass(const Selector &parentClassSelector) {
        auto *parent_class_ptr = _registry->FindClass<T>(parentClassSelector._fullname());
        // TODO: handle exception when parent is not a Class
        if (parent_class_ptr) {
            std::string fullname = _fullname();
            _evaluate_store([this, &fullname, &parent_class_ptr]() {
                _registry->RegisterClass<T>(fullname);
                auto *class_ptr = _registry->FindClass<T>(fullname);
                lua_getmetatable(_l, -1);
                class_ptr->CopyMembers(_l, *parent_class_ptr);
                lua_pop(_l, 1);
            });
        }
    }

    // Add class members
    template <typename T, typename... Members>
    void AddClassMembers(Members... members) {
        auto *class_ptr = _registry->FindClass<T>(_fullname());
        // TODO: handle exception when this element is not a Class
        if (class_ptr) {
            ResetStackOnScopeExit save(_l);
            _traverse_full();
            lua_getmetatable(_l, -1);
            class_ptr->RegisterMembers(_l, members...);
        }
    }

    // Add class ctor
    template <typename T, typename... CtorArgs>
    void AddClassCtor(const char *name = "__call") {
        auto *class_ptr = _registry->FindClass<T>(_fullname());
        // TODO: handle exception when this element is not a Class
        if (class_ptr) {
            ResetStackOnScopeExit save(_l);
            _traverse_full();
            lua_getmetatable(_l, -1);
            class_ptr->template RegisterCtor<CtorArgs...>(_l, name);
        }
    }

    // Retrieve a tuple that is returned by call a functor
    template <typename... Ret>
    std::tuple<Ret...> GetTuple() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(sizeof...(Ret));
        return detail::_get_n<Ret...>(_l);
    }

    // Retrieve a reference value that is returned by call a functor
    template <typename T, typename = std::enable_if_t<!detail::is_primitive_decay<T>::value>>
    operator T&() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<T&>{}, _l);
    }

    // Retrieve a pointer value that is returned by call a functor
    template <typename T>
    operator T*() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<T*>{}, _l);
    }

    // Retrieve a Reference value that is returned by call a functor
    template <typename T>
    operator Reference<T>() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<Reference<T>>{}, _l);
    }

    // Retrieve a Pointer value that is returned by call a functor
    template <typename T>
    operator Pointer<T>() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<Pointer<T>>{}, _l);
    }

    // Retrieve a bool value that is returned by call a functor
    operator bool() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<bool>{}, _l);
    }

    // Retrieve a signed integer value that is returned by call a functor
    operator int() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<int>{}, _l);
    }

    // Retrieve a unsigned integer value that is returned by call a functor
    operator unsigned int() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<unsigned int>{}, _l);
    }

    // Retrieve a float value that is returned by call a functor
    operator float() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<float>{}, _l);
    }

    // Retrieve a double value that is returned by call a functor
    operator double() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<double>{}, _l);
    }

    // Retrieve a std::string value that is returned by call a functor
    operator std::string() const {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        return detail::_pop(detail::_id<std::string>{}, _l);
    }

    // Retrieve a LuaCpp::function value that is returned by call a functor
    template <typename Ret, typename... Args>
    operator LuaCpp::function<Ret(Args...)>() {
        ResetStackOnScopeExit save(_l);
        _evaluate_retrieve(1);
        auto ret = detail::_pop(detail::_id<LuaCpp::function<Ret(Args...)>>{}, _l);
        ret._enable_exception_handler(_exception_handler);
        return ret;
    }

    // Chaining operators. If the selector is an rvalue, modify in
    // place. Otherwise, create a new Selector and return it.
#ifdef HAS_REF_QUALIFIERS
    Selector &&operator[](const std::string &name) && {
        _name += std::string(".") + name;
        _check_create_table();
        _traversal.push_back(_key);
        _key = make_Ref(_l, name);
        return std::move(*this);
    }

    Selector &&operator[](const char *name) && {
        return std::move(*this)[std::string(name)];
    }

    Selector &&operator[](const int index) && {
        _name += std::string(".") + std::to_string(index);
        _check_create_table();
        _traversal.push_back(_key);
        _key = make_Ref(_l, index);
        return std::move(*this);
    }
#endif // HAS_REF_QUALIFIERS
    Selector operator[](const std::string &name) const REF_QUAL_LVALUE {
        auto fullname = _name + "." + name;
        _check_create_table();
        auto traversal = _traversal;
        traversal.push_back(_key);
        return Selector(_l, _module, *_registry, *_exception_handler, fullname, traversal, make_Ref(_l, name));
    }

    Selector operator[](const char *name) const REF_QUAL_LVALUE {
        return (*this)[std::string(name)];
    }

    Selector operator[](const int index) const REF_QUAL_LVALUE {
        auto name = _name + "." + std::to_string(index);
        _check_create_table();
        auto traversal = _traversal;
        traversal.push_back(_key);
        return Selector(_l, _module, *_registry, *_exception_handler, name, traversal, make_Ref(_l, index));
    }

    template <typename T>
    void Enumerate(T callback) const {
        ResetStackOnScopeExit save(_l);
        _traverse_full();
        auto traversal = _traversal;
        traversal.push_back(_key);

        // push first key nil for enumeration
        lua_pushnil(_l);  
        while (lua_next(_l, -2)) {
            // uses 'key' (at index -2) and 'value' (at index -1)
            int key_type = lua_type(_l, -2);
            // only key type number and string are allowed
            if (key_type == LUA_TNUMBER) {
                const int index = (int)lua_tointeger(_l, -2);
                Selector selctor = (*this)[index];
                callback(selctor);
            } else if (key_type == LUA_TSTRING) {
                const std::string key = lua_tostring(_l, -2);
                Selector selector = (*this)[key];
                callback(selector);
            }
            // removes 'value'; keeps 'key' for next iteration
            lua_pop(_l, 1);
        }
    }

    int LuaType() const {
        ResetStackOnScopeExit save(_l);
        _traverse_full();
        return lua_type(_l, -1);
    }

    bool IsFunction() const {
        ResetStackOnScopeExit save(_l);
        _traverse_full();
        return lua_type(_l, -1) == LUA_TFUNCTION;
    }

    bool IsTable() const {
        ResetStackOnScopeExit save(_l);
        _traverse_full();
        return lua_type(_l, -1) == LUA_TTABLE;
    }

private:
    Selector(lua_State *s, Module *module, Registry &r, ExceptionHandler &eh, const std::string &name)
        : _l(s), _module(module), _registry(&r), _exception_handler(&eh), _name(name), _key(make_Ref(s, name)) {}

    Selector(lua_State *s, Module *module, Registry &r, ExceptionHandler &eh, const std::string &name, std::vector<LuaRef> traversal, LuaRef key)
        : _l(s), _module(module), _registry(&r), _exception_handler(&eh), _name(name), _traversal(traversal), _key(key) {}

    std::string _fullname() const;

    // Pushes key element to the stack
    // ex) table[key]
    void _get(const LuaRef &key) const {
        // NOTE: Assume that table is already pushed
        // Push table[key]
        key.Push(_l);
        lua_gettable(_l, -2);        
    }

    // Pushes traversed element
    void _traverse() const {
        if (!_module) {
#if LUA_VERSION_NUM >= 502
            lua_pushglobaltable(_l);
#else
            lua_pushvalue(_l, LUA_GLOBALSINDEX);
#endif
        } else {
            assert(lua_type(_l, -1) == LUA_TTABLE);
            // When local table pushed, copy it to prevent remove
            lua_pushvalue(_l, -1);
        }
        for (auto &key : _traversal) {
            // Get table element recursively
            _get(key);
            // Remove parent table
#if LUA_VERSION_NUM >= 502
            lua_remove(_l, lua_absindex(_l, -2));
#else
            lua_remove(_l, lua_gettop(_l) - 1);
#endif
        }
    }

    // Pushes full traversed element
    void _traverse_full() const {
        _traverse();
        _get(_key);
        if (!_module) {
            // Remove last parent table
#if LUA_VERSION_NUM >= 502
            lua_remove(_l, lua_absindex(_l, -2));
#else
            lua_remove(_l, lua_gettop(_l) - 1);
#endif        
        }
    }

    // Sets this element from a function that pushes a value to the stack.
    // ex) table[key] = pushFunc()
    template <typename PushFunc>
    void _put(PushFunc &&pushFunc) const {
        // NOTE: Assume that table is already pushed
        // table[key] = value that pushed from function execution
        _key.Push(_l);
        pushFunc();
        lua_settable(_l, -3);
    }

    // Sets a value of the traversed element from a function that pushes it to the stack
    // ex) tra.ver.sal[key] = pushFunc()
    template <typename PushFunc>
    void _evaluate_store(PushFunc &&pushFunc) const {
        ResetStackOnScopeExit save(_l);
        _traverse();
        _put(std::forward<PushFunc>(pushFunc));
    }

    // Call a functor if it is activated by operator()
    void _call_functor(int num_ret) const;    

    // Call a fully traversed functor if it is activated by operator()
    void _evaluate_retrieve(int num_ret) const {
        _traverse_full();
        _call_functor(num_ret);
    }

    // Check if full traversal key table exist, if it's not create new table
    void _check_create_table() const {
        ResetStackOnScopeExit save(_l);
        _traverse_full();

        if (!lua_istable(_l, -1)) { // not table
            lua_pop(_l, 1); // flush the stack
            _traverse();
            _put([this]() {
                lua_newtable(_l);
            });
        }
    }

    lua_State *_l;
    Registry *_registry;
    ExceptionHandler *_exception_handler;
    std::string _name;

    Module *_module;

    // Traverses the structure up to this element
    std::vector<LuaRef> _traversal;

    // String key of the value to act upon.
    LuaRef _key;

    // Functor is activated when the () operator is invoked.
    mutable MovingFlag _functor_active;

    // Functor arguments is preserved in registry
    std::vector<LuaRef> _functor_arguments;
};

// Comparison with a string and a retrieved value
inline bool operator==(const Selector &s, const char *cstr) {
    return std::string(cstr) == s.operator std::string();
}

inline bool operator==(const char *cstr, const Selector &s) {
    return std::string(cstr) == s.operator std::string();
}

// Comparison with a general typed value and a retrieved value
template <typename T>
inline bool operator==(const Selector &s, T &&t) {
    return std::forward<T>(s) == t;
}

template <typename T>
inline bool operator==(T &&t, const Selector &s) {
    return std::forward<T>(s) == t;
}

}
