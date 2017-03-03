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
    Stack(int size);
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
