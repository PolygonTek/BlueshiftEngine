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

    LinkList

    Circular linked list template

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename T>
class LinkList {
public:
    /// Constructs emtpy link list.
    LinkList();

    /// Destructs
    ~LinkList();

                    /// Returns true if the list is empty.
    bool            IsListEmpty() const { return head->next == head; }

                    /// Returns true if the node is in a list.
                    /// If called on the head of a list, will always return false.
    bool            InList() const { return head != this; }

                    /// Returns number of nodes.
    int             Count() const;

                    /// If node is the head of the list, clears the list.
                    /// Otherwise it just removes the node from the list.
    void            Clear();

                    /// Inserts this node before node node
    void            InsertBefore(LinkList &node);

                    /// Inserts this node after node node
    void            InsertAfter(LinkList &node);

                    /// Adds this node to the end of node node
    void            AddToEnd(LinkList &node) { InsertBefore(*node.head); }

                    /// Adds this node to the front of node node
    void            AddToFront(LinkList &node) { InsertAfter(*node.head); }

                    /// Removes this node in link list
    void            Remove();

                    /// Returns owner pointer of next node. 
                    /// Returns nullptr if this node is a last one
    T *             Next() const;

                    /// Returns owner pointer of previous node. 
                    /// Returns nullptr if this node is a first one
    T *             Prev() const;

                    /// Returns owner pointer of this node.
    T *             Owner() const { return owner; }

                    /// Sets owner pointer of this node.
    void            SetOwner(T *object) { owner = object; }

                    /// Returns head node pointer.
    LinkList *      ListHead() const { return head; }

                    /// Returns next node pointer.
                    /// Returns nullptr if this node is a last one
    LinkList *      NextNode() const { if (next == head) { return nullptr; } return next; }

                    /// Returns previous node pointer.
                    /// Returns nullptr if this node is a first one
    LinkList *      PrevNode() const { if (prev == head) { return nullptr; } return prev; }

private:
    LinkList *      head;   ///< Head pointer of circular node list. If empty list, head == next == prev
    LinkList *      next;   ///< Next pointer of circular node list
    LinkList *      prev;   ///< Previous pointer of circular node list
    T *             owner;  ///< Owner pointer
};

// 끊어진 node 는 자기 자신을 가리키는 head node
template <typename T>
LinkList<T>::LinkList() {
    head    = this;
    next    = this;
    prev    = this;
    owner   = nullptr;
}

template <typename T> 
LinkList<T>::~LinkList() {
    Clear();
}

template <typename T>
int LinkList<T>::Count() const {
    int num = 0;
    for (LinkList<T> *node = head->next; node != head; node = node->next) {
        num++;
    }

    return num;
}

template <typename T>
void LinkList<T>::Clear() {
    if (head == this) {
        // head node 라면 연결된 모든 node 를 제거
        while (next != this) {
            next->Remove();
        }
    } else {
        // list 에서 this node 제거
        Remove();
    }
}

template <typename T>
void LinkList<T>::Remove() {
    prev->next = next;
    next->prev = prev;

    next = this;
    prev = this;
    head = this;
}

template <typename T>
void LinkList<T>::InsertBefore(LinkList &node) {
    Remove();

    next        = &node;
    prev        = node.prev;
    node.prev   = this;
    prev->next  = this;
    head        = node.head;
}

template <typename T> 
void LinkList<T>::InsertAfter(LinkList &node) {
    Remove();

    prev        = &node;
    next        = node.next;
    node.next   = this;
    next->prev  = this;
    head        = node.head;
}

template <typename T> 
T *LinkList<T>::Next() const {
    if (!next || (next == head)) {
        return nullptr;
    }
    return next->owner;
}

template <typename T> 
T *LinkList<T>::Prev() const {
    if (!prev || (prev == head)) {
        return nullptr;
    }
    return prev->owner;
}

BE_NAMESPACE_END
