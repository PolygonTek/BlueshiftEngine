// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <utility>

/*
-------------------------------------------------------------------------------

    Pair

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Pair class template
template <typename T, typename U>
class Pair {
public:
    /// The default constructor.
    Pair() : first(T()), second(U()) {}

    /// Copy constructor.
    Pair(const Pair &rhs) = default;

    /// Constructs from rvalue pair.
    Pair(Pair &&rhs) = default;

    /// Constructs a pair with two values.
    Pair(const T &first, const U &second) : first(first), second(second) {}

    template <typename T1, typename T2> Pair(const Pair<T1, T2> &rhs) : first(rhs.first), second(rhs.second) {}
    template <typename T1, typename T2> Pair(Pair<T1, T2> &&rhs) : first(std::move(rhs.first)), second(std::move(rhs.second)) {}
    template <typename T1, typename T2> Pair(T1 &&f, T2 &&s) : first(std::forward<T1>(f)), second(std::forward<T2>(s)) {}

                /// Copy from another pair.
    Pair &     operator=(const Pair &rhs) = default;

                /// Move from another pair.
    Pair &     operator=(Pair &&rhs) = default;

    template <typename T1, typename T2> Pair &operator=(const Pair<T1, T2> &rhs);
    template <typename T1, typename T2> Pair &operator=(Pair<T1, T2> &&rhs);

                /// Tests for equality with another pair.
    bool        operator==(const Pair<T, U> &rhs) const { return first != rhs.first || second != rhs.second ? false : true; }

                /// Tests for inequality with another pair.
    bool        operator!=(const Pair<T, U> &rhs) const { return first != rhs.first || second != rhs.second ? true : false; }
    
                /// Tests for less than with another pair.
    bool        operator<(const Pair<T, U>& rhs) const;

                /// Tests for greater than with another pair.
    bool        operator>(const Pair<T, U>& rhs) const;

    T           first;      ///< First value.
    U           second;     ///< Second value.
};

template <typename T, typename U>
template <typename T1, typename T2>
BE_INLINE Pair<T, U> &Pair<T, U>::operator=(const Pair<T1, T2> &rhs) {
    first = rhs.first;
    second = rhs.second;
    return *this;
}

template <typename T, typename U>
template <typename T1, typename T2>
BE_INLINE Pair<T, U> &Pair<T, U>::operator=(Pair<T1, T2> &&rhs) {
    first = std::move(rhs.first);
    second = std::move(rhs.second);
    return *this;
}

template <typename T, typename U>
BE_INLINE bool Pair<T, U>::operator<(const Pair<T, U>& rhs) const {
    if (first < rhs.first) {
        return true;
    }
    if (first != rhs.first) {
        return false;
    }
    return second < rhs.second;
}

template <typename T, typename U>
BE_INLINE bool Pair<T, U>::operator>(const Pair<T, U>& rhs) const {
    if (first > rhs.first) {
        return true;
    }
    if (first != rhs.first) {
        return false;
    }
    return second > rhs.second;
}

BE_NAMESPACE_END
