#pragma once

namespace LuaCpp {

struct BaseObj {
    virtual ~BaseObj() {}
};

template <typename T>
class Obj : public BaseObj {
public:
    Obj(lua_State *l, T *object) {
        lua_newtable(l);
    }

    // Register a memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, T *object, const char *memberName, M T::*member) {
        RegisterMember(l, object, memberName, member, typename std::is_const<M>::type{});
    }

    // Register a non-const memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, T *object, const char *memberName, M T::*member, std::false_type) {
        std::function<M()> lambda_get = [object, member]() {
            return object->*member;
        };
        std::string getName = memberName;
        auto getFunc = new ObjFunc<1, M>(l, getName, lambda_get);
        auto it_get = _funcs.find(getName);
        if (it_get != _funcs.end()) {
            it_get->second = std::unique_ptr<BaseFunc>(getFunc);
        } else {
            _funcs.insert(std::make_pair(getName, std::unique_ptr<BaseFunc>(getFunc)));
        }

        std::function<void(M)> lambda_set = [object, member](M value) {
            object->*member = value;
        };
        std::string setName = std::string("set_") + memberName;
        auto setFunc = new ObjFunc<0, void, M>(l, setName, lambda_set);
        auto it_set = _funcs.find(setName);
        if (it_set != _funcs.end()) {
            it_set->second = std::unique_ptr<BaseFunc>(setFunc);
        } else {
            _funcs.insert(std::make_pair(setName, std::unique_ptr<BaseFunc>(setFunc)));
        }
    }

    // Register a const memeber variable
    template <typename M>
    void RegisterMember(lua_State *l, T *object, const char *memberName, M T::*member, std::true_type) {
        std::function<M()> lambda_get = [object, member]() {
            return object->*member;
        };
        auto objFunc = new ObjFunc<1, M>(l, std::string(memberName), lambda_get);
        auto it = _funcs.find(memberName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(objFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(memberName), std::unique_ptr<BaseFunc>(objFunc)));
        }
    }

    // Register a memeber function with perfect forwarding 
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, T *object, const char *funcName, Ret(T::*func)(Args&&...)) {
        std::function<Ret(Args&&...)> lambda = [object, func](Args&&... args) -> Ret {
            return (object->*func)(std::forward<Args>(args)...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto objFunc = new ObjFunc<arity, Ret, Args...>(l, std::string(funcName), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(objFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(objFunc)));
        }
    }

    // Register a non-const memeber function
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, T *object, const char *funcName, Ret(T::*func)(Args...)) {
        std::function<Ret(Args...)> lambda = [object, func](Args... args) -> Ret {
            return (object->*func)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto objFunc = new ObjFunc<arity, Ret, Args...>(l, std::string(funcName), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(objFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(objFunc)));
        }
    }

    // Register a const memeber function
    template <typename Ret, typename... Args>
    void RegisterMember(lua_State *l, T *object, const char *funcName, Ret(T::*func)(Args...) const) {
        std::function<Ret(Args...)> lambda = [object, func](Args... args) -> Ret {
            return (object->*func)(args...);
        };
        constexpr int arity = detail::_arity<Ret>::value;
        auto objFunc = new ObjFunc<arity, Ret, Args...>(l, std::string(funcName), lambda);
        auto it = _funcs.find(funcName);
        if (it != _funcs.end()) {
            it->second = std::unique_ptr<BaseFunc>(objFunc);
        } else {
            _funcs.insert(std::make_pair(std::string(funcName), std::unique_ptr<BaseFunc>(objFunc)));
        }
    }

    // Register multiple member functions
    template <typename M, typename... Ms>
    void RegisterMembers(lua_State *l, T *object, const char *name, M member, Ms... members) {
        RegisterMember(l, object, name, member);
        RegisterMembers(l, object, members...);
    }

    void RegisterMembers(lua_State *l, T *object) {}

private:
    std::unordered_map<std::string, std::unique_ptr<BaseFunc>> _funcs;
};

}
