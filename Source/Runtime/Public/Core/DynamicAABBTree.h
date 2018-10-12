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

    Dynamic AABB Tree

-------------------------------------------------------------------------------
*/

#include "Containers/Stack.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class BE_API DynamicAABBTree {
public:
    DynamicAABBTree();
    ~DynamicAABBTree();

    void            Purge(bool clearNodes = false);

                    /// Create a proxy. Provide a tight fitting AABB and userData pointer.
    int32_t         CreateProxy(const AABB &aabb, float expansion, void *userData);

                    /// Destory a proxy. This asserts if the id is invalid.
    void            DestroyProxy(int32_t proxyId);

                    /// Move a proxy with a swepted AABB. If the proxy has moved outside of its fattened AABB,
                    /// then the proxy is removed from the tree and re-inserted. Otherwise
                    /// the function returns immediately. 
    bool            MoveProxy(int32_t proxyId, const AABB &aabb, float expansion, const Vec3 &displacement);

                    /// Get proxy user data.
    void *          GetUserData(int32_t proxyId) const;

                    /// Get the fat AABB for a proxy.
    const AABB &    GetFatAABB(int32_t proxyId) const;

    const AABB &    GetRootFatAABB() const;

                    /// Validate this tree. For testing.
    void            Validate() const;

                    /// Compute the height of the binary tree in O(N) time. Should not be called often.
    int             GetHeight() const;

                    /// Get the maximum balance of an node in the tree. The balance is the difference
                    /// in height of the two children of a node.
    int             GetMaxBalance() const;

                    /// Get the ratio of the sum of the node areas to the root area.
    float           GetAreaRatio() const;

                    /// Build an optimal tree. Very expensive. For testing.
    void            RebuildBottomUp();

    template <typename F>
    void            Query(const Sphere &boundingVolume, F &callback) const;
    template <typename F>
    void            Query(const AABB &boundingVolume, F &callback) const;
    template <typename F>
    void            Query(const OBB &boundingVolume, F &callback) const;
    template <typename F>
    void            Query(const Frustum &boundingVolume, F &callback) const;

private:
    int             AllocNode();
    void            FreeNode(int32_t node);

    void            InsertLeaf(int32_t node);
    void            RemoveLeaf(int32_t node);

    int             Balance(int32_t index);

    int             ComputeHeight() const;
    int             ComputeHeight(int32_t nodeId) const;

    void            ValidateStructure(int32_t index) const;
    void            ValidateMetrics(int32_t index) const;

    struct Node {
        bool        IsLeaf() const { return child1 == -1; }
        
        AABB        aabb;
        void *      userData;
        int32_t     child1;
        int32_t     child2;
        int32_t     height;     // leaf = 0, free node = -1

        union {
            int32_t parent;
            int32_t next;
        };
    };

    int32_t         root;
    int32_t         nodeCount;
    int32_t         nodeCapacity;
    int32_t         freeList;
    Node *          nodes;
    int             insertionCount;
};

BE_INLINE void *DynamicAABBTree::GetUserData(int32_t proxyId) const {
    assert(0 <= proxyId && proxyId < nodeCapacity);
    return nodes[proxyId].userData;
}

BE_INLINE const AABB &DynamicAABBTree::GetFatAABB(int32_t proxyId) const {
    assert(0 <= proxyId && proxyId < nodeCapacity);
    return nodes[proxyId].aabb;
}

BE_INLINE const AABB &DynamicAABBTree::GetRootFatAABB() const {
    if (root == -1) {
        return AABB::zero;
    }
    return GetFatAABB(root); 
}

template <typename F>
BE_INLINE void DynamicAABBTree::Query(const Sphere &sphere, F &callback) const {
    Stack<int32_t> stack(256);
    stack.Push(root);

    while (!stack.IsEmpty()) {
        int32_t nodeId = stack.Pop();
        if (nodeId == -1) {
            continue;
        }

        const Node *node = nodes + nodeId;

        if (sphere.IsIntersectAABB(node->aabb)) {
            if (node->IsLeaf()) {
                bool proceed = callback(nodeId);
                if (proceed == false) {
                    return;
                }
            } else {
                stack.Push(node->child1);
                stack.Push(node->child2);
            }
        }
    }
}

template <typename F>
BE_INLINE void DynamicAABBTree::Query(const AABB &aabb, F &callback) const {
    Stack<int32_t> stack(256);
    stack.Push(root);

    while (!stack.IsEmpty()) {
        int32_t nodeId = stack.Pop();
        if (nodeId == -1) {
            continue;
        }

        const Node *node = nodes + nodeId;

        if (aabb.IsIntersectAABB(node->aabb)) {
            if (node->IsLeaf()) {
                bool proceed = callback(nodeId);
                if (proceed == false) {
                    return;
                }
            } else {
                stack.Push(node->child1);
                stack.Push(node->child2);
            }
        }
    }
}

template <typename F>
BE_INLINE void DynamicAABBTree::Query(const OBB &obb, F &callback) const {
    Stack<int32_t> stack(256);
    stack.Push(root);

    while (!stack.IsEmpty()) {
        int32_t nodeId = stack.Pop();
        if (nodeId == -1) {
            continue;
        }

        const Node *node = nodes + nodeId;
        
        if (obb.IsIntersectOBB(OBB(node->aabb))) {
            if (node->IsLeaf()) {
                bool proceed = callback(nodeId);
                if (proceed == false) {
                    return;
                }
            } else {
                stack.Push(node->child1);
                stack.Push(node->child2);
            }
        }
    }
}

template <typename F>
BE_INLINE void DynamicAABBTree::Query(const Frustum &frustum, F &callback) const {
    Stack<int32_t> stack(256);
    stack.Push(root);

    while (!stack.IsEmpty()) {
        int32_t nodeId = stack.Pop();
        if (nodeId == -1) {
            continue;
        }

        const Node *node = nodes + nodeId;

        if (!frustum.CullAABB(node->aabb)) {
            if (node->IsLeaf()) {
                bool proceed = callback(nodeId);
                if (proceed == false) {
                    return;
                }
            } else {
                stack.Push(node->child1);
                stack.Push(node->child2);
            }
        }
    }
}

BE_NAMESPACE_END
