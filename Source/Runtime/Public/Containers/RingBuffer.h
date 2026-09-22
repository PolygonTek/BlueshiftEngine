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
#include <climits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

/*
---------------------------------------------------------------------------------------------------

    Ring Buffer

    Dynamic FIFO with reusable storage. Synchronization is the caller's responsibility.
    Only live elements are constructed. Pointer elements do not own their pointees.
    Growing invalidates references; logical index 0 always refers to the oldest element.

---------------------------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename T>
class RingBuffer {
public:
    RingBuffer() = default;
    explicit RingBuffer(int initialCapacity) { Reserve(initialCapacity); }
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;
    RingBuffer(RingBuffer &&rhs) noexcept { Swap(rhs); }
    RingBuffer &operator=(RingBuffer &&rhs) noexcept;
    ~RingBuffer() { Clear(); }

    bool                IsEmpty() const { return count == 0; }
    bool                IsFull() const { return count == capacity; }

    int                 Count() const { return count; }
    int                 Capacity() const { return capacity; }

                        /// Reserves exactly newCapacity slots when growing, preserving FIFO order.
    void                Reserve(int newCapacity);

                        /// Destroys all live elements while retaining the allocated storage.
    void                Reset();

                        /// Destroys all live elements and releases the allocated storage.
    void                Clear();

    void                Swap(RingBuffer &rhs) noexcept;

    T &                 Front() { assert(count > 0); return elements[readIndex]; }
    const T &           Front() const { assert(count > 0); return elements[readIndex]; }

    T &                 Back() { return (*this)[count - 1]; }
    const T &           Back() const { return (*this)[count - 1]; }

    T &                 operator[](int index);
    const T &           operator[](int index) const;

                        /// Appends an element, doubling capacity if necessary. Never overwrites.
    template <typename... Args>
    T &                 EmplaceBack(Args &&...args);
    T &                 PushBack(const T &value) { return EmplaceBack(value); }
    T &                 PushBack(T &&value) { return EmplaceBack(std::move(value)); }

                        /// Returns false when full, without allocating or consuming the arguments.
    template <typename... Args>
    [[nodiscard]] bool  TryEmplaceBack(Args &&...args);
    [[nodiscard]] bool  TryPushBack(const T &value) { return TryEmplaceBack(value); }
    [[nodiscard]] bool  TryPushBack(T &&value) { return TryEmplaceBack(std::move(value)); }

                        /// Removes and returns the oldest element. Requires a nonempty buffer.
    T                   PopFront();

private:
    int                 PhysicalIndex(int index) const;

    template <typename... Args>
    T &                 EmplaceBackUnchecked(Args &&...args);

    template <typename... Args>
    T &                 GrowAndEmplaceBack(Args &&...args);

    static constexpr int DefaultCapacity = 16;

    T *                 elements = nullptr;
    int                 capacity = 0;
    int                 count = 0;
    int                 readIndex = 0;
};

template <typename T>
RingBuffer<T> &RingBuffer<T>::operator=(RingBuffer &&rhs) noexcept {
    if (this != &rhs) {
        Clear();
        Swap(rhs);
    }
    return *this;
}

template <typename T>
void RingBuffer<T>::Swap(RingBuffer &rhs) noexcept {
    std::swap(elements, rhs.elements);
    std::swap(capacity, rhs.capacity);
    std::swap(count, rhs.count);
    std::swap(readIndex, rhs.readIndex);
}

template <typename T>
int RingBuffer<T>::PhysicalIndex(int index) const {
    // Avoid modulo and overflow in readIndex + index.
    const int remaining = capacity - readIndex;
    return index < remaining ? readIndex + index : index - remaining;
}

template <typename T>
T &RingBuffer<T>::operator[](int index) {
    assert(index >= 0 && index < count);
    return elements[PhysicalIndex(index)];
}

template <typename T>
const T &RingBuffer<T>::operator[](int index) const {
    assert(index >= 0 && index < count);
    return elements[PhysicalIndex(index)];
}

template <typename T>
void RingBuffer<T>::Reserve(int newCapacity) {
    assert(newCapacity >= 0);
    if (newCapacity <= capacity) {
        return;
    }

    RingBuffer expanded;
    // Raw storage supports non-default-constructible and over-aligned element types.
    expanded.elements = std::allocator<T>().allocate(newCapacity);
    expanded.capacity = newCapacity;
    for (int i = 0; i < count; ++i) {
        expanded.EmplaceBackUnchecked(std::move_if_noexcept((*this)[i]));
    }
    Swap(expanded);
}

template <typename T>
void RingBuffer<T>::Reset() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (int i = 0; i < count; ++i) {
            elements[PhysicalIndex(i)].~T();
        }
    }
    count = 0;
    readIndex = 0;
}

template <typename T>
void RingBuffer<T>::Clear() {
    Reset();
    if (elements) {
        std::allocator<T>().deallocate(elements, capacity);
        elements = nullptr;
    }
    capacity = 0;
}

template <typename T>
template <typename... Args>
T &RingBuffer<T>::EmplaceBackUnchecked(Args &&...args) {
    T *element = ::new (static_cast<void *>(elements + PhysicalIndex(count))) T(std::forward<Args>(args)...);
    ++count;
    return *element;
}

template <typename T>
template <typename... Args>
T &RingBuffer<T>::EmplaceBack(Args &&...args) {
    if (IsFull()) {
        return GrowAndEmplaceBack(std::forward<Args>(args)...);
    }
    return EmplaceBackUnchecked(std::forward<Args>(args)...);
}

template <typename T>
template <typename... Args>
bool RingBuffer<T>::TryEmplaceBack(Args &&...args) {
    if (IsFull()) {
        return false;
    }
    EmplaceBackUnchecked(std::forward<Args>(args)...);
    return true;
}

template <typename T>
template <typename... Args>
T &RingBuffer<T>::GrowAndEmplaceBack(Args &&...args) {
    if (capacity == INT_MAX) {
        throw std::length_error("RingBuffer capacity exceeded");
    }
    const int newCapacity = capacity == 0 ? DefaultCapacity : (capacity > INT_MAX / 2 ? INT_MAX : capacity * 2);
    RingBuffer expanded(newCapacity);

    // Construct first: arguments may reference an element in the current buffer.
    T *element = ::new (static_cast<void *>(expanded.elements + count)) T(std::forward<Args>(args)...);
    try {
        for (int i = 0; i < count; ++i) {
            expanded.EmplaceBackUnchecked(std::move_if_noexcept((*this)[i]));
        }
    } catch (...) {
        // expanded owns the constructed prefix; the appended element is separate until success.
        element->~T();
        throw;
    }
    ++expanded.count;
    Swap(expanded);
    return *element;
}

template <typename T>
T RingBuffer<T>::PopFront() {
    T value(std::move_if_noexcept(Front()));
    elements[readIndex].~T();
    if (++readIndex == capacity) {
        readIndex = 0;
    }
    --count;
    return value;
}

BE_NAMESPACE_END
