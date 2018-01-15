// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/*
-------------------------------------------------------------------------------=

    Stack template 
    
    using Array (see Array.h) internally

-------------------------------------------------------------------------------=
*/

#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

template <typename T>
class Stack {
public:
    /// Constructs stack with the given stack size.
    Stack(int size);

    /// Constructs stack with the given stack size and granularity.
    Stack(int size, int granularity);
    
    const T &           operator[](int index) const;
    T &                 operator[](int index);

    int                 Count() const;

    bool                IsEmpty() const;

    T &                 Push();
    T &                 Push(const T &element);
    const T             Pop();

    const T &           Top() const;
    T &                 Top();

    void                Clear();
    void                DeleteContents(bool clear);
    
    void                Swap(Stack &rhs);

private:
    Array<T>            stack;
};

template <typename T>
BE_INLINE Stack<T>::Stack(int size) {
    stack.Resize(size, size);
}

template <typename T>
BE_INLINE Stack<T>::Stack(int size, int granularity) {
    stack.Resize(size, granularity);
}

template <typename T>
BE_INLINE const T &Stack<T>::operator[](int index) const {
    assert(index >= 0 && index < stack.Count());
    return stack[index];
}

template <typename T>
BE_INLINE T &Stack<T>::operator[](int index) {
    assert(index >= 0 && index < stack.Count());
    return stack[index];
}

template <typename T>
BE_INLINE int Stack<T>::Count() const {
    return stack.Count(); 
}

template <typename T>
BE_INLINE bool Stack<T>::IsEmpty() const {
    return stack.Count() == 0;
}

template <typename T>
BE_INLINE T &Stack<T>::Push() {
    return stack.Alloc();
}

template <typename T>
BE_INLINE T &Stack<T>::Push(const T &element) {
    T &ref = stack.Alloc();
    ref = element;
    return ref;
}

template <typename T>
BE_INLINE const T Stack<T>::Pop() {
    T val = stack.TakeLast();
    return val;
}

template <typename T>
BE_INLINE const T &Stack<T>::Top() const {
    return stack[Count() - 1];
}

template <typename T>
BE_INLINE T &Stack<T>::Top() {
    return stack[Count() - 1];
}

template <typename T>
BE_INLINE void Stack<T>::Clear() {
    stack.Clear();
}

template <typename T>
BE_INLINE void Stack<T>::DeleteContents(bool clear) {
    stack.DeleteContents(clear);
}

template <typename T>
BE_INLINE void Stack<T>::Swap(Stack &rhs) {
    stack.Swap(rhs.stack);
}

BE_NAMESPACE_END
