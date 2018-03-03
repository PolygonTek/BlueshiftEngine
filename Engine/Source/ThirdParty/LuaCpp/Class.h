#pragma once

namespace LuaCpp {

struct BaseClass {
    virtual ~BaseClass() {}
};

template <typename T>
class Class : public BaseClass {
public:
    Class(lua_State *l, const std::string &name) 
        : _name(name) {
        // Pushes the new table for this class (actual result)
        lua_newtable(l); // (1)
        // Each classes has it's own metatable
        _metatable_name = _name + "_meta_class";
        MetatableRegistry::PushNewMetatable(l, typeid(T), _metatable_name); // (2)
        // Class members are stored in the metatable 
        // Register default ctor with no arguments
        RegisterDefaultCtor(l);
        RegisterDtor(l);
        // Set __index field of metatable to it's own
        lua_pushvalue(l, -1); // copy metatable onto top (3)
        lua_setfield(l, -2, "__index"); // pops the value on top (2)
        // Set metatable of a this table
        lua_setmetatable(l, -2); // pops the metatable on top (1)
    }
    ~Class() = default;
    Class(const Class &) = delete;
    Class(Class &&other) = default;
    Class &operator=(const Class &) = delete;
    Class &operator=(Class &&other) = default;    

    // Register ctor
    template <typename... Args, int N = 0>
    typename std::enable_if<N == 0 && !std::is_abstract<T>::value>::type RegisterCtor(lua_State *l, const char *ctorName) {
        auto ctor = new Ctor<T, Args...>(l, std::string(ctorName), _metatable_name.c_str());
        auto it = _ctors.find(ctorName);
        if (it != _ctors.end()) {
            it->second = std::unique_ptr<BaseFunc>(ctor);
        } else {
            _ctors.insert(std::make_pair(std::string(ctorName), std::unique_ptr<BaseFunc>(ctor)));
        }
    }

    // SFINAE for abstract class
    template <typename... Args, int N = 0>
    typename std::enable_if<N == 0 && std::is_abstract<T>::value>::type RegisterCtor(lua_State *l, const char *ctorName) {
        // do nothing
    }

    // Register default ctor
    template <int N = 0>
    typename std::enable_if<N == 0 && std::is_default_constructible<T>::value>::type RegisterDefaultCtor(lua_State *l) {
        RegisterCtor(l, "__call");
    }

    // SFINAE for the class that is not default constructible
    template <int N = 0>
    typename std::enable_if<N == 0 && !std::is_default_constructible<T>::value>::type RegisterDefaultCtor(lua_State *l) {
        // do nothing
    }

    // Register a memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, const char *memberName, M T::*member) {
        RegisterMember(l, memberName, member, typename std::is_const<M>::type{});
    }

    // Register a non-const memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, const char *memberName, M T::*member, std::false_type) {
        std::function<M(T*)> lambda_get = [member](T *self) {
            return self->*member;
        };
        std::string getName = memberName;
        auto getFunc = new ClassFunc<1, T, M>(l, getName, _metatable_name.c_str(), lambda_get);
        auto it_get = _funcs.find(getName);
        if (it_get != _funcs.end()) {
            it_get->second = std::unique_ptr<BaseFunc>(getFunc);
        } else {
            _funcs.insert(std::make_pair(getName, std::unique_ptr<BaseFunc>(getFunc)));
        }

        std::function<void(T*, M)> lambda_set = [member](T *self, M value) {
            (self->*member) = value;
        };
        std::string setName = std::string("set_") + memberName;
        auto setFunc = new ClassFunc<0, T, void, M>(l, setName, _metatable_name.c_str(), lambda_set);
        auto it_set = _funcs.find(setName);
        if (it_set != _funcs.end()) {
            it_set->second = std::unique_ptr<BaseFunc>(setFunc);
        } else {
            _funcs.insert(std::make_pair(setName, std::unique_ptr<BaseFunc>(setFunc)));
        }
    }

    // Register a const memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, const char *memberName, M T::*member, std::true_type) {
        std::function<M(T*)> lambda_get = [member](T *self) {
            return self->*member;
        };
        auto classFunc = new ClassFunc<1, T, M>(l, std::string(memberName), _metatable_name.c_str(), lambda_get);
        auto it = _funcs.find(memberName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(classFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(memberName), std::unique_ptr<BaseFunc>(classFunc)));
        }
    }

    // Register a memeber function with perfect forwarding 
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, const char *funcName, Ret(T::*func)(Args&&...)) {
        // Convert memeber function call to non-member function call using lambda
        // so lambda(obj, a, b, c) means obj->func(a, b, c)
        std::function<Ret(T*, Args&&...)> lambda = [func](T *self, Args&&... args) -> Ret {
            return (self->*func)(std::forward<Args>(args)...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto classFunc = new ClassFunc<arity, T, Ret, Args...>(l, std::string(funcName), _metatable_name.c_str(), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(classFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(classFunc)));
        }
    }

    // Register a non-const memeber function
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, const char *funcName, Ret(T::*func)(Args...)) {
        // Convert memeber function call to non-member function call using lambda
        // so lambda(obj, a, b, c) means obj->func(a, b, c)
        std::function<Ret(T*, Args...)> lambda = [func](T *self, Args... args) -> Ret {
            return (self->*func)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto classFunc = new ClassFunc<arity, T, Ret, Args...>(l, std::string(funcName), _metatable_name.c_str(), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(classFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(classFunc)));
        }
    }

    // Register a const memeber function
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, const char *funcName, Ret(T::*func)(Args...) const) {
        // Convert memeber function call to non-member function call using lambda
        // so lambda(obj, a, b, c) means obj->func(a, b, c)
        std::function<Ret(const T*, Args...)> lambda = [func](const T *self, Args... args) -> Ret {
            return (self->*func)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto classFunc = new ClassFunc<arity, const T, Ret, Args...>(l, std::string(funcName), _metatable_name.c_str(), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(classFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(classFunc)));
        }
    }

    // Register a const lvalue ref-qualified memeber function
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, const char *funcName, Ret(T::*func)(Args...) const &) {
        // Convert memeber function call to non-member function call using lambda
        // so lambda(obj, a, b, c) means obj->func(a, b, c)
        std::function<Ret(const T*, Args...)> lambda = [func](const T *self, Args... args) -> Ret {
            return (self->*func)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto classFunc = new ClassFunc<arity, const T, Ret, Args...>(l, std::string(funcName), _metatable_name.c_str(), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(classFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(classFunc)));
        }
    }

    // Register multiple member functions
    template <typename Member, typename... NameMembers>
    void RegisterMembers(lua_State *l, const char *name, Member member, NameMembers... nameMembers) {
        RegisterMember(l, name, member);
        RegisterMembers(l, nameMembers...);
    }

    void RegisterMembers(lua_State *l) {}

    void CopyMembers(lua_State *l, const Class &parentClass) {
        for (const auto &parentFunc : parentClass._funcs) {
            auto clonedFunc = parentFunc.second->Clone(l, _metatable_name);
            auto it = _funcs.find(parentFunc.first);
            if (it != _funcs.end()) {
                it->second = std::unique_ptr<BaseFunc>(clonedFunc);
            } else {
                _funcs.insert(std::make_pair(parentFunc.first, std::unique_ptr<BaseFunc>(clonedFunc)));
            }
        }
    }

private:
    // Register dtor
    template <int N = 0>
    typename std::enable_if<N == 0 && !std::is_abstract<T>::value>::type RegisterDtor(lua_State *l) {
        _dtor.reset(new Dtor<T>(l, _metatable_name.c_str()));
    }

    // SFINAE for abstract class
    template <int N = 0>
    typename std::enable_if<N == 0 && std::is_abstract<T>::value>::type RegisterDtor(lua_State *l) {
        // do nothing
    }

    std::string _name;
    std::string _metatable_name;
    std::unique_ptr<Dtor<T>> _dtor;
    std::unordered_map<std::string, std::unique_ptr<BaseFunc>> _ctors;
    std::unordered_map<std::string, std::unique_ptr<BaseFunc>> _funcs;
};

}
