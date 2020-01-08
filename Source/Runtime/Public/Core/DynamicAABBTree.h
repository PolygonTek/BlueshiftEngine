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

    A dynamic AABB tree arranges data in a binary tree to accelerate queries 
    such as volume queries and ray casts. Leafs are proxies with an AABB. 
    In the tree we expand the proxy AABB by 'expansion' so that the proxy AABB 
    is bigger than the client object. This allows the client object to move by 
    small amounts without triggering a tree update.

    Nodes are pooled and relocatable, so we use node indices rather than pointers.

-------------------------------------------------------------------------------
*/

#include "Containers/Stack.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class BE_API DynamicAABBTree {
public:
    DynamicAABBTree();
    ~DynamicAABBTree();

                        /// Returns total size of allocated memory.
    size_t              Allocated() const { return nodeCapacity * sizeof(*nodes); }

                        /// Returns total size of allocated memory including size of this type.
    size_t              Size() const { return Allocated() + sizeof(*this); }

                        /// Clears all data and free memory in use.
    void                Clear();

                        /// Create a proxy.
                        /// Provide a tight fitting AABB and userData pointer.
    int32_t             CreateProxy(const AABB &aabb, float expansion, void *userData);

                        /// Destory a proxy.
                        /// This asserts if the id is invalid.
    void                DestroyProxy(int32_t proxyId);

                        /// Move a proxy with a swepted AABB. If the proxy has moved outside of its fattened AABB,
                        /// then the proxy is removed from the tree and re-inserted.
                        /// Otherwise the function returns immediately.
                        /// @return true if the proxy was re-inserted.
    bool                MoveProxy(int32_t proxyId, const AABB &aabb, float expansion, const Vec3 &displacement);

                        /// Get proxy user data.
                        /// @return the proxy user data or nullptr if the id is invalid.
    void *              GetUserData(int32_t proxyId) const;

                        /// Get the fat AABB for a proxy.
    const AABB &        GetFatAABB(int32_t proxyId) const;

                        /// Get the fat AABB of the root node.
    const AABB &        GetRootFatAABB() const;

                        /// Query an bounding volume for overlapping proxies.
                        /// The callback functor is called for each proxy that overlaps the supplied bounding volume.
    template <typename F>
    void                Query(const Sphere &boundingVolume, F &callback) const;
    template <typename F>
    void                Query(const AABB &boundingVolume, F &callback) const;
    template <typename F>
    void                Query(const OBB &boundingVolume, F &callback) const;
    template <typename F>
    void                Query(const Frustum &boundingVolume, F &callback) const;

    template <typename F>
    void                QueryDepthRange(int depthMin, int depthMax, F &callback) const;

                        /// Compute the height of the binary tree in O(N) time.
                        /// Should not be called often.
    int                 GetHeight() const;

                        /// Get the maximum balance of an node in the tree.
                        /// The balance is the difference in height of the two children of a node.
    int                 GetMaxBalance() const;

                        /// Get the ratio of the sum of the node areas to the root area.
    float               GetAreaRatio() const;

                        /// Build an optimal tree.
                        /// Very expensive. For testing.
    void                RebuildBottomUp();

                        /// Validate this tree. For testing.
    void                Validate() const;

private:
    int                 AllocNode();
    void                FreeNode(int32_t node);

    void                InsertLeaf(int32_t node);
    void                RemoveLeaf(int32_t node);

    int                 Balance(int32_t index);

    int                 ComputeHeight() const;
    int                 ComputeHeight(int32_t nodeId) const;

    void                ValidateStructure(int32_t index) const;
    void                ValidateMetrics(int32_t index) const;

    template <typename F>
    void                QueryDepthRangeRecursive(int nodeId, int depthMin, int depthMax, int depth, F &callback) const;

    struct Node {
        bool            IsLeaf() const { return child1 == -1; }
       
        AABB            aabb;               // AABB enclosing this node
        void *          userData;           // user data pointer
        int32_t         child1;             // child node index
        int32_t         child2;             // child node index
        int32_t         height;             // leaf = 0, free node = -1

        union {
            int32_t     parent;
            int32_t     next;
        };
    };

    int32_t             root;
    int32_t             nodeCount;
    int32_t             nodeCapacity;
    int32_t             freeList;
    Node *              nodes = nullptr;
    int                 insertionCount;
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
        return AABB::empty;
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

template <typename F>
BE_INLINE void DynamicAABBTree::QueryDepthRangeRecursive(int nodeId, int depthMin, int depthMax, int depth, F &callback) const {
    if (nodeId == -1) {
        return;
    }

    if (depth > depthMax) {
        return;
    }

    if (depth >= depthMin) {
        callback(nodeId);
    }

    const Node *node = &nodes[nodeId];

    if (!node->IsLeaf()) {
        depth++;

        if (depth > depthMax) {
            return;
        }

        QueryDepthRangeRecursive(node->child1, depthMin, depthMax, depth, callback);
        QueryDepthRangeRecursive(node->child2, depthMin, depthMax, depth, callback);
    }
}

template <typename F>
BE_INLINE void DynamicAABBTree::QueryDepthRange(int depthMin, int depthMax, F &callback) const {
    QueryDepthRangeRecursive(root, depthMin, depthMax, 0, callback);
}

BE_NAMESPACE_END
