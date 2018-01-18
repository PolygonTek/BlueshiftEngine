#pragma once

namespace LuaCpp {

namespace detail {
    template <typename T>
    struct is_primitive {
        static constexpr bool value = std::is_enum<T>::value;
    };

    template <>
    struct is_primitive<char> {
        static constexpr bool value = true;
    };
    
    template <>
    struct is_primitive<signed char> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<unsigned char> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<short> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<unsigned short> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<int> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<unsigned int> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<long> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<unsigned long> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<long long> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<unsigned long long> {
        static constexpr bool value = true;
    };
    
    template <>
    struct is_primitive<bool> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<float> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<double> {
        static constexpr bool value = true;
    };

    template <>
    struct is_primitive<std::string> {
        static constexpr bool value = true;
    };

    template <typename T>
    struct is_primitive_decay {
        static constexpr bool value = is_primitive<std::decay_t<T>>::value;
    };

    // Type decay only for the primitive type
    template <typename T>
    using decay_if_primitive = typename std::conditional<is_primitive<typename std::decay<T>::type>::value, typename std::decay<T>::type, T>::type;

    // value is 1 in case of single value
    template <typename T>
    struct _arity {
        static constexpr int value = 1;
    };

    // value is N in case of a size-N tuple
    template <typename... Vs>
    struct _arity<std::tuple<Vs...>> {
        static constexpr int value = sizeof...(Vs);
    };

    // value is 0 in case of void
    template <>
    struct _arity<void> {
        static constexpr int value = 0;
    };

    template <typename T, typename = void>
    struct is_callable : std::is_function<T> {};

    template <typename T>
    struct is_callable<T, std::enable_if_t<std::is_same<decltype(void(&T::operator())), void>::value>> : std::true_type {};

    #if 1 // NOTE: use std::make_index_sequence and std::index_sequence instead of below codes

    // indices "tag" for use incremental indices tag struct which is like _indices<0, 1, 2, ...>
    template <std::size_t... Is>
    struct _indices {};

    // Recursively inherits from itself until the base case (template arguments which is start with 0)
    // We can make tag like _indices_builder<4>::type() == _indices<0, 1, 2, 3>()
    template <std::size_t N, std::size_t... Is>
    struct _indices_builder : _indices_builder<N-1, N-1, Is...> {};

    // The base case where we define the type tag
    template <std::size_t... Is>
    struct _indices_builder<0, Is...> {
        using type = _indices<Is...>;
    };

    #endif

    template <typename T> struct _id {};
}

}
