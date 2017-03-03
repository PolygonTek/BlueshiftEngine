#pragma once

namespace LuaCpp {

template <typename... T>
class Tuple {
public:
    Tuple(T&... args) : _tuple(args...) {}

    void operator=(const LuaCpp::Selector &s) {
        _tuple = s.GetTuple<typename std::remove_reference_t<T>...>();
    }

private:
    std::tuple<T&...> _tuple;
};

template <typename... T>
inline Tuple<T&...> tie(T&... args) {
    return Tuple<T&...>(args...);
}

}
