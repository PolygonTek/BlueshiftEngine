#pragma once

namespace LuaCpp {

class MovingFlag {
public:
    MovingFlag() = default;
    MovingFlag(MovingFlag const &) = default;
    MovingFlag &operator=(MovingFlag const &) = default;
    MovingFlag(MovingFlag && that) noexcept : flag(that.flag) {
        that = false;
    }

    MovingFlag &operator=(MovingFlag && that) noexcept {
        this->flag = that.flag;
        that = false;
        return *this;
    }

    operator bool() const {
        return flag;
    }

    MovingFlag &operator=(bool x) {
        flag = x;
        return *this;
    }

private:
    bool flag = false;
};

class ResetStackOnScopeExit {
public:
    explicit ResetStackOnScopeExit(lua_State * stack)
        : _stack(stack), _saved_top_index(lua_gettop(_stack)) {}

    ~ResetStackOnScopeExit() {
        if (_stack) {
            lua_settop(_stack, _saved_top_index);
        }
    }

    ResetStackOnScopeExit(ResetStackOnScopeExit const &) = delete;
    ResetStackOnScopeExit(ResetStackOnScopeExit &&) = delete;
    ResetStackOnScopeExit &operator=(ResetStackOnScopeExit const &) = delete;
    ResetStackOnScopeExit &operator=(ResetStackOnScopeExit &&) = delete;

private:
    lua_State *_stack;
    int _saved_top_index;
};

}
