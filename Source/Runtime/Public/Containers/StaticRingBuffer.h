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

#include <cassert>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

/*
---------------------------------------------------------------------------------------------------

    Static Ring Buffer

    Fixed-capacity FIFO with embedded storage. Synchronization is the caller's responsibility.
    Only live elements are constructed. Pointer elements do not own their pointees.
    All capacity slots are usable; logical index 0 always refers to the oldest element.
    Moving takes linear time and empties the source on success.
    If an element move throws, existing buffers remain valid but their contents may change.

---------------------------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename T, int capacity>
class StaticRingBuffer {
    static_assert(capacity > 0, "StaticRingBuffer capacity must be positive");

public:
    StaticRingBuffer() {} // Keep unused storage uninitialized, including during value-initialization.
    StaticRingBuffer(const StaticRingBuffer &) = delete;
    StaticRingBuffer &operator=(const StaticRingBuffer &) = delete;
    StaticRingBuffer(StaticRingBuffer &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>);
    StaticRingBuffer &operator=(StaticRingBuffer &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>);

    ~StaticRingBuffer() { Reset(); }

    bool                IsEmpty() const { return count == 0; }
    bool                IsFull() const { return count == capacity; }

    int                 Count() const { return count; }
    int                 Capacity() const { return capacity; }

                        /// Destroys all live elements. The embedded storage and capacity are retained.
    void                Reset();
    void                Clear() { Reset(); }

    T &                 Front() { assert(count > 0); return *ElementAt(readIndex); }
    const T &           Front() const { assert(count > 0); return *ElementAt(readIndex); }

    T &                 Back() { return (*this)[count - 1]; }
    const T &           Back() const { return (*this)[count - 1]; }

    T &                 operator[](int index);
    const T &           operator[](int index) const;

                        /// Appends an element, throwing std::length_error when full. Never overwrites.
    template <typename... Args>
    T &                 EmplaceBack(Args &&...args);
    T &                 PushBack(const T &value) { return EmplaceBack(value); }
    T &                 PushBack(T &&value) { return EmplaceBack(std::move(value)); }

                        /// Returns false when full, without constructing an element or consuming arguments.
    template <typename... Args>
    [[nodiscard]] bool  TryEmplaceBack(Args &&...args);
    [[nodiscard]] bool  TryPushBack(const T &value) { return TryEmplaceBack(value); }
    [[nodiscard]] bool  TryPushBack(T &&value) { return TryEmplaceBack(std::move(value)); }

                        /// Removes and returns the oldest element. Requires a nonempty buffer.
    T                   PopFront();

private:
    int                 PhysicalIndex(int index) const;
    T *                 ElementAt(int physicalIndex);
    const T *           ElementAt(int physicalIndex) const;

    template <typename... Args>
    T &                 EmplaceBackUnchecked(Args &&...args);
    void                MoveFrom(StaticRingBuffer &rhs);

    // Separate aligned slots avoid default-constructing T or treating raw storage as a T array.
    struct Slot {
        alignas(T) unsigned char bytes[sizeof(T)];
    };

    Slot                elements[capacity];
    int                 count = 0;
    int                 readIndex = 0;
};

template <typename T, int capacity>
StaticRingBuffer<T, capacity>::StaticRingBuffer(StaticRingBuffer &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
    : StaticRingBuffer() {
    // Delegation ensures live elements are destroyed if an element's move constructor throws.
    MoveFrom(rhs);
}

template <typename T, int capacity>
StaticRingBuffer<T, capacity> &StaticRingBuffer<T, capacity>::operator=(StaticRingBuffer &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (this != &rhs) {
        Reset();
        MoveFrom(rhs);
    }
    return *this;
}

template <typename T, int capacity>
void StaticRingBuffer<T, capacity>::MoveFrom(StaticRingBuffer &rhs) {
    for (int i = 0; i < rhs.count; ++i) {
        EmplaceBackUnchecked(std::move(rhs[i]));
    }
    rhs.Reset();
}

template <typename T, int capacity>
int StaticRingBuffer<T, capacity>::PhysicalIndex(int index) const {
    // Avoid modulo and overflow in readIndex + index.
    const int remaining = capacity - readIndex;
    return index < remaining ? readIndex + index : index - remaining;
}

template <typename T, int capacity>
T *StaticRingBuffer<T, capacity>::ElementAt(int physicalIndex) {
    return std::launder(reinterpret_cast<T *>(elements[physicalIndex].bytes));
}

template <typename T, int capacity>
const T *StaticRingBuffer<T, capacity>::ElementAt(int physicalIndex) const {
    return std::launder(reinterpret_cast<const T *>(elements[physicalIndex].bytes));
}

template <typename T, int capacity>
T &StaticRingBuffer<T, capacity>::operator[](int index) {
    assert(index >= 0 && index < count);
    return *ElementAt(PhysicalIndex(index));
}

template <typename T, int capacity>
const T &StaticRingBuffer<T, capacity>::operator[](int index) const {
    assert(index >= 0 && index < count);
    return *ElementAt(PhysicalIndex(index));
}

template <typename T, int capacity>
void StaticRingBuffer<T, capacity>::Reset() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (int i = 0; i < count; ++i) {
            ElementAt(PhysicalIndex(i))->~T();
        }
    }
    count = 0;
    readIndex = 0;
}

template <typename T, int capacity>
template <typename... Args>
T &StaticRingBuffer<T, capacity>::EmplaceBackUnchecked(Args &&...args) {
    T *element = ::new (static_cast<void *>(elements[PhysicalIndex(count)].bytes)) T(std::forward<Args>(args)...);
    ++count;
    return *element;
}

template <typename T, int capacity>
template <typename... Args>
T &StaticRingBuffer<T, capacity>::EmplaceBack(Args &&...args) {
    if (IsFull()) {
        throw std::length_error("StaticRingBuffer capacity exceeded");
    }
    return EmplaceBackUnchecked(std::forward<Args>(args)...);
}

template <typename T, int capacity>
template <typename... Args>
bool StaticRingBuffer<T, capacity>::TryEmplaceBack(Args &&...args) {
    if (IsFull()) {
        return false;
    }
    EmplaceBackUnchecked(std::forward<Args>(args)...);
    return true;
}

template <typename T, int capacity>
T StaticRingBuffer<T, capacity>::PopFront() {
    T value(std::move_if_noexcept(Front()));
    ElementAt(readIndex)->~T();
    if (++readIndex == capacity) {
        readIndex = 0;
    }
    --count;
    return value;
}

BE_NAMESPACE_END
