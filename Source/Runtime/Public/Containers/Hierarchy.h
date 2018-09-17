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

                        /// Removes the node from parent
    void                RemoveFromParent();

                        /// Removes the node from the hierarchy and adds it's children to the parent.
    void                RemoveFromHierarchy();

                        /// Root of this node
    T *                 GetRoot() const;
                        /// Parent of this node
    T *                 GetParent() const;
                        /// First child of this node
    T *                 GetChild() const;
                        /// Next node with the same parent
    T *                 GetNextSibling() const;
                        /// Previous node with the same parent. Returns nullptr if no parent, or if it is the first child
    T *                 GetPrevSibling() const;

                        /// Get next node in depth-first order
    T *                 GetNext() const;
                        /// Get next leaf in depth-first order
    T *                 GetNextLeaf() const;

private:
    Hierarchy *         parent;
    Hierarchy *         nextSibling;
    Hierarchy *         child;
    T *                 owner;

                        /// Previous node with the same parent. Returns nullptr if no parent, or if it is the first child
    Hierarchy<T> *      GetPrevSiblingNode() const;
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
    nextSibling = node.child;
    node.child = this;
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
T *Hierarchy<T>::GetRoot() const {
    const Hierarchy *p = this;
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
T *Hierarchy<T>::GetChild() const {
    if (child) {
        return child->owner;
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
T *Hierarchy<T>::GetNext() const {
    const Hierarchy<T> *node;

    if (child) {
        return child->owner;
    } else {
        node = this;
        while (node && node->nextSibling == nullptr) {
            node = node->parent;
        }

        if (node) {
            return node->nextSibling->owner;
        } else {
            return nullptr;
        }
    }
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
    } else {
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
        } else {
            return nullptr;
        }
    }
}

BE_NAMESPACE_END
