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
-------------------------------------------------------------------------------

    StaticArray

    Templated static array

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Templated static array
template <typename T, int capacity>
class StaticArray {
    static_assert(std::is_default_constructible<T>::value, "StaticArray requires default-constructible elements");

public:
    /// Constructs empty array.
    StaticArray();

    /// Constructs from another array.
    StaticArray(const StaticArray<T, capacity> &array);
    
    /// Aggregates initialization constructor.
    StaticArray(const std::initializer_list<T> &array);
    
    /// Destructs.
    ~StaticArray() {}

                    /// Returns true if the array has count 0; otherwise returns false.
    bool            IsEmpty() const { return count == 0; }

                    /// Number of elements.
    int             Count() const { return count; }

                    /// Returns capacity of this array.
    int             Capacity() const { return capacity; }

                    /// Returns total size of allocated memory.
    size_t          Allocated() const { return capacity * sizeof(T); }

                    /// Returns total size of allocated memory including size of this type.
    size_t          Size() const { return sizeof(StaticArray<T, capacity>) + Allocated(); }
    
                    /// Returns actual size of the used elements.
    size_t          MemoryUsed() const { return count * sizeof(elements[0]); }

                    /// Returns a pointer to the elements stored in the array.
                    /// The pointer can be used to access and modify the items in the array.
    T *             Ptr() { return &elements[0]; }

                    /// Returns a const pointer to the data stored in the array.
                    /// The pointer can be used to access the items in the array.
    const T *       Ptr() const { return &elements[0]; }

                    /// Returns the item at 'index' position as a constant reference.
                    /// 'index' must be a valid index position in the array (i.e., 0 <= index < Count()).
    const T &       operator[](int index) const;

                    /// Returns the item at 'index' position as a modifiable reference.
                    /// 'index' must be a valid index position in the array (i.e., 0 <= index < Count()).
    T &             operator[](int index);

                    /// Returns the first item.
    const T &       First() const { assert(count > 0); return elements[0]; }

                    /// Returns the first item.
    T &             First() { assert(count > 0); return elements[0]; }

                    /// Returns the last item.
    const T &       Last() const { assert(count > 0); return elements[count - 1]; }

                    /// Returns the last item.
    T &             Last() { assert(count > 0); return elements[count - 1]; }

                    /// Compares with another array.
    bool            operator==(const StaticArray<T, capacity> &rhs) const;
    bool            operator!=(const StaticArray<T, capacity> &rhs) const;

                    /// Removes all the elements from the array.
    void            Clear();

                    /// Deletes each elements. 
                    /// This is valid operation only for pointer typed array.
                    /// param @clear call clear function.
    void            DeleteContents(bool clear);

                    /// Sets number of elements.
    void            SetCount(int newCount);

                    /// Appends new element and returns pointer of it.
                    /// Returns nullptr if failed to append.
    T *             Alloc();

                    /// Inserts 'value' at index position 'index'.
                    /// Returns index of the inserted element or -1 if failed to append.
    template <typename CompatibleT>
    int             Insert(CompatibleT &&value, int index = 0);

                    /// Inserts the items of the array 'array' at index position 'index'.
                    /// Returns index of the last inserted element.
    int             InsertArray(const StaticArray<T, capacity> &array, int index = 0);

                    /// Appends 'value' at the end of the array.
                    /// Returns index of the last appended element.
    template <typename CompatibleT, typename... Rest>
    int             Append(CompatibleT &&value, Rest&&... rest);
    int             Append() { return count - 1; }

                    /// Appends the items of the array 'array' to this array.
                    /// Returns index of the last appended element or -1 if failed to append.
    int             AppendArray(const StaticArray<T, capacity> &array);

                    /// Appends the unique value 'value'.
                    /// Nothing happens if value 'value' is in the array already.
                    /// Returns index of the element or -1 if failed to append.
    template <typename CompatibleT>
    int             AddUnique(CompatibleT &&value);

                    /// Returns the index position of the first occurrence of 'value' in the array, searching forward from index position 'from'.
                    /// Returns -1 if no item matched.
    template <typename CompatibleT>
    int             FindIndex(CompatibleT &&value, int from = 0) const;
                    /// Returns the index position of the first matching element for which predicate 'finder' returns true in the array, searching forward from index position 'from'.
                    /// Returns -1 if no item matched.
    template <typename Functor>
    int             FindIndexIf(Functor &&finder, int from = 0);

                    /// Returns the element pointer of the first occurence of 'value' in the array, searching forward from index position 'from'.
                    /// Returns nullptr if no item matched.
    template <typename CompatibleT>
    T *             Find(CompatibleT &&value, int from = 0) const;
                    /// Returns the element pointer of the first matching element for which predicate 'finder' returns true in the array, searching forward from index position 'from'.
                    /// Returns nullptr if no item matched.
    template <typename Functor>
    T *             FindIf(Functor &&finder, int from = 0);

                    /// Returns the index position of the first occurence of nullptr.
                    /// This is valid operation only for pointer typed array.
    int             FindNull() const;

                    /// Returns the index position of the first occurence of non-nullptr.
                    /// This is valid operation only for pointer typed array.
    int             FindNotNull() const;

                    /// Calculate the index using valid element pointer.
                    /// This is valid operation only for pointer typed list.
                    // 메모리 포인터끼리 빼는 연산이므로 index 가 항상 array 에 존재하는 것은 아니다. 
                    // 범위를 넘어가는 index 일 경우 debug 모드에서는 assert 에러가 발생한다.
    int             IndexOf(const T *value) const;

                    /// Removes the element at index position 'index'.
                    // 제거되고 이후의 element 들이 빈공간을 메꾸기 위해 앞으로 이동된다.
                    /// Returns false if failed to remove.
    bool            RemoveIndex(int index);

                    /// Removes the first element that compares equal to 'value' from the array.
                    /// Returns whether an element was, in fact, removed.
                    // 제거되고 이후의 element 들이 빈공간을 메꾸기 위해 앞으로 이동된다.
    template <typename CompatibleT>
    bool            Remove(CompatibleT &&value);

                    /// Removes all elements that compare equal to 'value' from the array.
                    /// Returns the number of elements removed.
    template <typename CompatibleT>
    bool            RemoveAll(CompatibleT &&value);

                    /// Sorts using predicate 'compare'.
    template <typename Functor>
    void            Sort(Functor &&compare);
    void            Sort() { Sort(std::less<T>()); }

                    /// Sorts sub-section using predicate 'compare'.
    template <typename Functor>
    void            SortSubSection(int startIndex, int endIndex, Functor &&compare);
    void            SortSubSection(int startIndex, int endIndex) { SortSubSection(startIndex, endIndex, std::less<T>()); }

                    /// Swaps array 'array' with this array.
    void            Swap(StaticArray<T, capacity> &array);

private:
    int             count;              ///< Number of elements in use
    T               elements[capacity]; ///< Array of elements
};

template <typename T, int capacity>
BE_INLINE StaticArray<T, capacity>::StaticArray() {
    count = 0;
}

template <typename T, int capacity>
BE_INLINE StaticArray<T, capacity>::StaticArray(const StaticArray<T, capacity> &array) {
    *this = array;
}

template <typename T, int capacity>
BE_INLINE StaticArray<T, capacity>::StaticArray(const std::initializer_list<T> &array) : StaticArray() {
    Reserve(array.size());

    for (auto it = array.begin(); it != array.end(); ++it) {
        Append(*it);
    }
}

template <typename T, int capacity>
BE_INLINE void StaticArray<T, capacity>::Clear() {
    count = 0;
}

template <typename T, int capacity>
BE_INLINE void StaticArray<T, capacity>::DeleteContents(bool clear) {
    for (int i = 0; i < count; i++) {
        delete elements[i];
        elements[i] = nullptr;
    }

    if (clear) {
        Clear();
    } else {
        memset(elements, 0, sizeof(elements));
    }
}

template <typename T, int capacity>
BE_INLINE void StaticArray<T, capacity>::SetCount(int newCount) {
    assert(newCount >= 0);
    assert(newCount <= capacity);
    count = newCount;
}

template <typename T, int capacity>
BE_INLINE const T &StaticArray<T, capacity>::operator[](int index) const {
    assert(index >= 0);
    assert(index < capacity);

    return elements[index];
}

template <typename T, int capacity>
BE_INLINE T &StaticArray<T, capacity>::operator[](int index) {
    assert(index >= 0);
    assert(index < capacity);

    return elements[index];
}

template <typename T, int capacity>
BE_INLINE bool StaticArray<T, capacity>::operator==(const StaticArray<T, capacity> &rhs) const {
    if (count != rhs.count) {
        return false;
    }

    for (int i = 0; i < count; i++) {
        if (elements[i] != rhs.elements[i]) {
            return false;
        }
    }

    return true;
}

template <typename T, int capacity>
BE_INLINE bool StaticArray<T, capacity>::operator!=(const StaticArray<T, capacity> &rhs) const {
    if (count != rhs.count) {
        return true;
    }

    for (int i = 0; i < count; i++) {
        if (elements[i] != rhs.elements[i]) {
            return true;
        }
    }

    return false;
}

template <typename T, int capacity>
BE_INLINE T *StaticArray<T, capacity>::Alloc() {
    if (count >= capacity) {
        return nullptr;
    }

    return elements[count++];
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE int StaticArray<T, capacity>::Insert(CompatibleT &&value, int index) {
    assert(count < capacity);
    assert(index >= 0 && index <= count);

    if (count >= capacity) {
        return -1;
    }

    for (int i = count; i > index; --i) {
        elements[i] = elements[i - 1];
    }

    elements[index] = std::forward<CompatibleT>(value);
    count++;

    return index;
}

template <typename T, int capacity>
BE_INLINE int StaticArray<T, capacity>::InsertArray(const StaticArray<T, capacity> &array, int index) {
    assert(index >= 0 && index + array.count <= count);

    for (int i = count - 1; i >= index; --i) {
        elements[i + array.count] = elements[i];
    }

    for (int i = 0; i < array.count; i++) {
        elements[index + i] = array.elements[i];
    }

    count += array.count;

    return index + array.count - 1;
}

template <typename T, int capacity>
template <typename CompatibleT, typename... Rest>
BE_INLINE int StaticArray<T, capacity>::Append(CompatibleT &&value, Rest&&... rest) {
    Insert(std::forward<CompatibleT>(value), count);
    return Append(std::forward<Rest>(rest)...);
}

template <typename T, int capacity>
BE_INLINE int StaticArray<T, capacity>::AppendArray(const StaticArray<T, capacity> &array) {
    return InsertArray(array, count);
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE int StaticArray<T, capacity>::AddUnique(CompatibleT &&value) {
    int index = FindIndex(value);
    if (index < 0) {
        index = Append(std::forward<CompatibleT>(value));
    }

    return index;
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE int StaticArray<T, capacity>::FindIndex(CompatibleT &&value, int from) const {
    for (int i = from; i < count; i++) {
        if (elements[i] == std::forward<CompatibleT>(value)) {
            return i;
        }
    }

    return -1;
}

template <typename T, int capacity>
template <typename Functor>
BE_INLINE int StaticArray<T, capacity>::FindIndexIf(Functor &&finder, int from) {
    T *first = elements + from;
    T *last = elements + count;

    T *e = std::find_if(first, last, std::forward<Functor>(finder));
    if (e != last) {
        return (e - elements) / sizeof(T);
    }
    return -1;
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE T *StaticArray<T, capacity>::Find(CompatibleT &&value, int from) const {
    int i = FindIndex(std::forward<CompatibleT>(value), from);
    if (i >= 0) {
        return &elements[i];
    }

    return nullptr;
}

template <typename T, int capacity>
template <typename Functor>
BE_INLINE T *StaticArray<T, capacity>::FindIf(Functor &&finder, int from) {
    T *first = elements + from;
    T *last = elements + count;

    T *e = std::find_if(first, last, std::forward<Functor>(finder));
    if (e != last) {
        return e;
    }
    return nullptr;
}

template <typename T, int capacity>
BE_INLINE int StaticArray<T, capacity>::FindNull() const {
    for (int i = 0; i < count; i++) {
        if (elements[i] == nullptr) {
            return i;
        }
    }

    return -1;
}

template <typename T, int capacity>
BE_INLINE int StaticArray<T, capacity>::FindNotNull() const {
    for (int i = 0; i < count; i++) {
        if (elements[i] != nullptr) {
            return i;
        }
    }

    return -1;
}

template <typename T, int capacity>
BE_INLINE int StaticArray<T, capacity>::IndexOf(const T *objptr) const {
    int index = objptr - elements;

    assert(index >= 0);
    assert(index < count);

    return index;
}

template <typename T, int capacity>
BE_INLINE bool StaticArray<T, capacity>::RemoveIndex(int index) {
    assert(index >= 0);
    assert(index < count);

    if ((index < 0) || (index >= count)) {
        return false;
    }

    count--;
    for (int i = index; i < count; i++) {
        elements[i] = elements[i + 1];
    }

    return true;
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE bool StaticArray<T, capacity>::Remove(CompatibleT &&value) {
    int index = FindIndex(std::forward<CompatibleT>(value));
    if (index >= 0) {
        return RemoveIndex(index);
    }
    
    return false;
}

template <typename T, int capacity>
template <typename CompatibleT>
BE_INLINE bool StaticArray<T, capacity>::RemoveAll(CompatibleT &&value) {
    int removed = 0;
    int index = FindIndex(value, 0);

    while (index >= 0) {
        RemoveIndex(index);
        removed++;

        index = FindIndex(value, index);
    }

    return removed;
}

template <typename T, int capacity>
template <typename Functor>
BE_INLINE void StaticArray<T, capacity>::Sort(Functor &&compare) {
    std::sort(elements, elements + count, std::forward<Functor>(compare));
}

template <typename T, int capacity>
template <typename Functor>
BE_INLINE void StaticArray<T, capacity>::SortSubSection(int startIndex, int endIndex, Functor &&compare) {
    if (startIndex < 0) {
        startIndex = 0;
    }

    if (endIndex >= count) {
        endIndex = count - 1;
    }

    if (startIndex >= endIndex) {
        return;
    }

    std::sort(&elements[startIndex], &elements[endIndex], std::forward<Functor>(compare));
}

template <typename T, int capacity>
BE_INLINE void StaticArray<T, capacity>::Swap(StaticArray<T, capacity> &array) {
    StaticArray<T, capacity> temp = *this;
    *this = array;
    array = temp;
}

BE_NAMESPACE_END
