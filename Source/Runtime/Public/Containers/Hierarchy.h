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

    Hierarchy

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename T>
class Hierarchy {
public:
    Hierarchy();
    ~Hierarchy();

                        /// Returns the object that is associated with this node.
    T *                 Owner() const { return owner; }

                        /// Sets the object that this node is associated with.
    void                SetOwner(T *owner) { this->owner = owner; }

                        /// Returns true if the given node is parent of this node.
    bool                IsParentedBy(const Hierarchy &node) const;
                        
                        /// Makes the given node the parent.
    void                SetParent(Hierarchy &node);

                        /// Makes the given node a nextSibling after the passed in node.
    void                MakeSiblingAfter(Hierarchy &node);

                        /// Removes the node from parent.
    void                RemoveFromParent();

                        /// Removes the node from the hierarchy and adds it's children to the parent.
    void                RemoveFromHierarchy();

                        /// Returns number of children of this node.
    int                 GetChildCount() const;

                        /// Returns root of this node.
    T *                 GetRoot() const;

                        /// Returns parent of this node.
    T *                 GetParent() const;
   
                        /// Returns first child of this node.
    T *                 GetFirstChild() const;

                        /// Returns last child of this node.
    T *                 GetLastChild() const;

                        /// Returns child of this node with the given sibling index.
    T *                 GetChild(int index) const;

                        /// Returns next node with the same parent.
    T *                 GetNextSibling() const;

                        /// Returns previous node with the same parent. Returns nullptr if no parent, or if it is the first child.
    T *                 GetPrevSibling() const;

                        /// Returns the sibling index.
    int                 GetSiblingIndex() const;

                        /// Sets the sibling index.
    void                SetSiblingIndex(int index);

                        /// Moves this node to the start of the sibling list.
    void                SetAsFirstSibling();

                        /// Moves this node to the end of the sibling list.
    void                SetAsLastSibling();

                        /// Returns next node in depth-first order.
    T *                 GetNext() const;

                        /// Returns next leaf in depth-first order.
    T *                 GetNextLeaf() const;

                        /// Returns the depth-first order index.
    int                 GetIndex() const;

private:
    Hierarchy *         parent;
    Hierarchy *         nextSibling;
    Hierarchy *         child;
    T *                 owner;

                        /// Returns next node in depth-first order.
    Hierarchy *         GetNextNode() const;
                        /// Previous node with the same parent. Returns nullptr if no parent, or if it is the first child.
    Hierarchy *         GetPrevSiblingNode() const;
};

template <typename T>
Hierarchy<T>::Hierarchy() {
    owner       = nullptr;
    parent      = nullptr;
    nextSibling = nullptr;
    child       = nullptr;
}

template <typename T>
Hierarchy<T>::~Hierarchy() {
    RemoveFromHierarchy();
}

template <typename T>
bool Hierarchy<T>::IsParentedBy(const Hierarchy &node) const {
    if (parent == &node) {
        return true;
    } else if (parent) {
        return parent->IsParentedBy(node);
    }
    return false;
}

template <typename T>
void Hierarchy<T>::SetParent(Hierarchy &node) {
    RemoveFromParent();
    parent = &node;
    nextSibling = nullptr;

    Hierarchy<T> *currNode = node.child;
    Hierarchy<T> *prevNode = nullptr;
    while (currNode) {
        prevNode = currNode;
        currNode = currNode->nextSibling;
    }

    if (prevNode) {
        prevNode->nextSibling = this;
    } else {
        node.child = this;
    }
}

template <typename T>
void Hierarchy<T>::MakeSiblingAfter(Hierarchy &node) {
    RemoveFromParent();
    parent = node.parent;
    nextSibling = node.nextSibling;
    node.nextSibling = this;
}

template <typename T>
void Hierarchy<T>::RemoveFromParent() {
    if (parent) {
        Hierarchy<T> *prev = GetPrevSiblingNode();
        if (prev) {
            prev->nextSibling = nextSibling;
        } else {
            parent->child = nextSibling;
        }
    }

    parent = nullptr;
    nextSibling = nullptr;
}

template <typename T>
void Hierarchy<T>::RemoveFromHierarchy() {
    Hierarchy<T> *parentNode = parent;

    RemoveFromParent();

    if (parentNode) {
        while (child) {
            Hierarchy<T> *node = child;
            node->RemoveFromParent();
            node->SetParent(*parentNode);
        }
    } else {
        while (child) {
            child->RemoveFromParent();
        }
    }
}

template <typename T>
int Hierarchy<T>::GetChildCount() const {
    int count = 0;
    Hierarchy<T> *node = child;
    while (node) {
        count++;
        node = node->nextSibling;
    }
    return count;
}

template <typename T>
T *Hierarchy<T>::GetRoot() const {
    const Hierarchy<T> *p = this;
    while (p->parent) {
        p = p->parent;
    }
    return p->owner;
}

template <typename T>
T *Hierarchy<T>::GetParent() const {
    if (parent) {
        return parent->owner;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetFirstChild() const {
    if (child) {
        return child->owner;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetLastChild() const {
    int childCount = GetChildCount();
    if (childCount == 0) {
        return nullptr;
    }
    return GetChild(childCount - 1); 
}

template <typename T>
T *Hierarchy<T>::GetChild(int index) const {
    int currentIndex = 0;
    Hierarchy<T> *node = child;
    while (node) {
        if (currentIndex == index) {
            return node->owner;
        }
        node = node->nextSibling;
        currentIndex++;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetNextSibling() const {
    if (nextSibling) {
        return nextSibling->owner;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetPrevSibling() const {
    Hierarchy<T> *prior = GetPrevSiblingNode();
    if (prior) {
        return prior->owner;
    }
    return nullptr;
}

template <typename T>
Hierarchy<T> *Hierarchy<T>::GetPrevSiblingNode() const {
    if (!parent || parent->child == this) {
        return nullptr;
    }

    Hierarchy<T> *prev = nullptr;
    Hierarchy<T> *node = parent->child;

    while (node != this && node != nullptr) {
        prev = node;
        node = node->nextSibling;
    }

    if (node != this) {
        // could not find node in parent's list of children
        return nullptr; 
    }

    return prev;
}

template <typename T>
int Hierarchy<T>::GetSiblingIndex() const {
    if (!parent) {
        return -1;
    }
    int currentIndex = 0;
    Hierarchy<T> *node = parent->child;
    while (node) {
        if (node == this) {
            return currentIndex;
        }
        currentIndex++;
        node = node->nextSibling;
    }
    return -1;
}

template <typename T>
void Hierarchy<T>::SetSiblingIndex(int siblingIndex) {
    if (!parent) {
        return;
    }

    Hierarchy<T> *prev = GetPrevSiblingNode();
    if (prev) {
        prev->nextSibling = nextSibling;
    } else {
        parent->child = nextSibling;
    }

    if (siblingIndex == 0) {
        nextSibling = parent->child;
        parent->child = this;
        return;
    }

    int currentIndex = 1;
    Hierarchy<T> *node = parent->child;

    while (node) {
        if (currentIndex == siblingIndex) {
            nextSibling = node->nextSibling;
            node->nextSibling = this;
            return;
        }
        currentIndex++;
        node = node->nextSibling;
    }
}

template <typename T>
void Hierarchy<T>::SetAsFirstSibling() {
    SetSiblingIndex(0);
}

template <typename T>
void Hierarchy<T>::SetAsLastSibling() {
    SetSiblingIndex(parent->GetChildCount() - 1);
}

template <typename T>
int Hierarchy<T>::GetIndex() const {
    const Hierarchy<T> *rootNode = this;
    while (rootNode->parent) {
        rootNode = rootNode->parent;
    }

    int index = 0;
    for (const Hierarchy<T> *node = rootNode->GetNextNode(); node; node = node->GetNextNode()) {
        if (node == this) {
            return index;
        }
        index++;
    }
    return -1;
}

template <typename T>
Hierarchy<T> *Hierarchy<T>::GetNextNode() const {
    if (child) {
        return child;
    }

    const Hierarchy<T> *node = this;
    while (node && node->nextSibling == nullptr) {
        node = node->parent;
    }

    if (node) {
        return node->nextSibling;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetNext() const {
    Hierarchy<T> *nextNode = GetNextNode();
    if (nextNode) {
        return nextNode->owner;
    }
    return nullptr;
}

template <typename T>
T *Hierarchy<T>::GetNextLeaf() const {
    const Hierarchy<T> *node;

    if (child) {
        node = child;
        while (node->child) {
            node = node->child;
        }
        return node->owner;
    }
    node = this;
    while (node && node->nextSibling == nullptr) {
        node = node->parent;
    }
        
    if (node) {
        node = node->nextSibling;
        while (node->child) {
            node = node->child;
        }
        return node->owner;
    }
    return nullptr;
}

BE_NAMESPACE_END
