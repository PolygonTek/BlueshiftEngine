#pragma once

/*
-------------------------------------------------------------------------------

    Queue

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN
    
template <typename T>
class Queue {
public:
    Queue();

    void            Clear();

    bool            IsEmpty() { return first == nullptr; }

    void            Add(Queue &node);
    T *             RemoveFirst();
    T *             Peek() const;

    Queue *         GetFirst() const { return first; }
    Queue *         GetLast() const { return last; }
    Queue *         GetNext() const { return next; }

    void            SetNext(Queue *next) { this->next = next; }
    
    int             Count() const;

    T *             Owner() const { return owner; }
    void            SetOwner(T *owner) { this->owner = owner; }

private:
    Queue *         first;
    Queue *         last;
    Queue *         next;
    T *             owner;
};

template <typename T>
Queue<T>::Queue() {
    first = last = next = nullptr;
    owner = nullptr;
}

template <typename T>
void Queue<T>::Clear() {
    first = last = next = nullptr;
    owner = nullptr;
}

template <typename T>
void Queue<T>::Add(Queue &node) {
    node.SetNext(nullptr);
    if (last) {
        last->SetNext(&node);
    } else {
        first = &node;
    }
    last = &node;
}

template <typename T>
T *Queue<T>::RemoveFirst() {
    Queue *node = first;
    if (!node) {
        return nullptr;
    }

    first = first->GetNext();
    if (last == node) {
        last = nullptr;
    }
    node->SetNext(nullptr);
    return node->owner;
}

template <typename T>
T *Queue<T>::Peek() const {
    if (!first) {
        return nullptr;
    }
    return first->owner;
}

template <typename T>
int Queue<T>::Count() const {
    int num = 0;
    for (const Queue<T> *node = GetFirst(); node; node = node->GetNext()) {
        num++;
    }
    return num;
}

BE_NAMESPACE_END
