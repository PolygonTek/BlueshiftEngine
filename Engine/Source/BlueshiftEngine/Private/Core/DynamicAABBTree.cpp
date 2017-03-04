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

#include "Precompiled.h"
#include "Core/DynamicAABBTree.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

const int32_t   DEFAULT_CAPACITY            = 16;
const float     DISPLACEMENT_MULTIPLIER     = 2.0f;

DynamicAABBTree::DynamicAABBTree() {
    nodeCapacity = DEFAULT_CAPACITY;
    nodes = (Node *)Mem_Alloc(nodeCapacity * sizeof(nodes[0]));
    Purge();
}

DynamicAABBTree::~DynamicAABBTree() {
    Mem_Free(nodes);
}

void DynamicAABBTree::Purge(bool clearNodes) {
    if (clearNodes) {
        Mem_Free(nodes);
        nodeCapacity = DEFAULT_CAPACITY;
        nodes = (Node *)Mem_Alloc(nodeCapacity * sizeof(nodes[0]));
    }

    memset(nodes, 0, nodeCapacity * sizeof(nodes[0]));
    nodeCount = 0;
    
    // Build a linked list for the free list
    Node *node = nodes;
    for (int32_t i = 0; i < nodeCapacity - 1; i++, node++) {
        node->next = i + 1;
        node->height = -1;
    }
    node->next = -1;
    node->height = -1;
    freeList = 0;
    root = -1;
    insertionCount = 0;
}

// Allocated a node from the pool. Grow the pool if necessary.
int32_t DynamicAABBTree::AllocNode() {
    // Expand the node pool as needed.
    if (freeList == -1) {
        assert(nodeCount == nodeCapacity);

        // The free list is empty. Rebuild a bigger pool.
        Node *oldNodes = nodes;
        nodeCapacity *= 2;
        nodes = (Node *)Mem_Alloc(nodeCapacity * sizeof(nodes[0]));
        memcpy(nodes, oldNodes, nodeCount * sizeof(nodes[0]));
        Mem_Free(oldNodes);

        // Build a linked list for the free list. The parent
        // pointer becomes the "next" pointer.
        Node *node = &nodes[nodeCount];
        for (int32_t i = nodeCount; i < nodeCapacity - 1; i++, node++) {
            node->next = i + 1;
            node->height = -1;
        }
        node->next = -1;
        node->height = -1;
        freeList = nodeCount;
    }

    // Peel a node off the free list
    int32_t nodeId = freeList;
    Node *node = &nodes[freeList];
    freeList = node->next;
    node->parent = -1;
    node->child1 = -1;
    node->child2 = -1;
    node->height = 0;
    node->userData = nullptr;
    nodeCount++;
    return nodeId;
}

// Return a node to the pool
void DynamicAABBTree::FreeNode(int32_t nodeId) {
    assert(0 <= nodeId && nodeId < nodeCapacity);
    assert(0 < nodeCount);

    nodes[nodeId].next = freeList;
    nodes[nodeId].height = -1;
    freeList = nodeId;
    nodeCount--;
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow the node pool.
int32_t DynamicAABBTree::CreateProxy(const AABB &aabb, float expansion, void *userData) {
    int32_t proxyId = AllocNode();

    // Fatten the aabb.
    nodes[proxyId].aabb = aabb;
    nodes[proxyId].aabb.ExpandSelf(expansion);
    nodes[proxyId].userData = userData;
    nodes[proxyId].height = 0;

    InsertLeaf(proxyId);

    return proxyId;
}

void DynamicAABBTree::DestroyProxy(int32_t proxyId) {
    assert(0 <= proxyId && proxyId < nodeCapacity);
    assert(nodes[proxyId].IsLeaf());

    RemoveLeaf(proxyId);
    FreeNode(proxyId);
}

bool DynamicAABBTree::MoveProxy(int32_t proxyId, const AABB &aabb, float expansion, const Vec3 &displacement) {
    assert(0 <= proxyId && proxyId < nodeCapacity);
    assert(nodes[proxyId].IsLeaf());

    if (nodes[proxyId].aabb.IsContainAABB(aabb)) {
        return false;
    }

    RemoveLeaf(proxyId);

    // Expand AABB.
    AABB b = aabb;
    b.ExpandSelf(expansion);

    // Predict AABB displacement.
    Vec3 d = DISPLACEMENT_MULTIPLIER * displacement;

    if (d.x < 0.0f) {
        b[0].x += d.x;
    } else {
        b[1].x += d.x;
    }

    if (d.y < 0.0f) {
        b[0].y += d.y;
    } else {
        b[1].y += d.y;
    }

    if (d.z < 0.0f) {
        b[0].z += d.z;
    } else {
        b[1].z += d.z;
    }

    nodes[proxyId].aabb = b;

    InsertLeaf(proxyId);
    return true;
}

void DynamicAABBTree::InsertLeaf(int32_t leaf) {
    insertionCount++;

    if (root == -1) {
        root = leaf;
        nodes[root].parent = -1;
        return;
    }

    // Find the best sibling for this node
    AABB leafAABB = nodes[leaf].aabb;
    int32_t index = root;
    while (nodes[index].IsLeaf() == false) {
        int32_t child1 = nodes[index].child1;
        int32_t child2 = nodes[index].child2;

        float area = nodes[index].aabb.Area();

        AABB combinedAABB = nodes[index].aabb + leafAABB;
        float combinedArea = combinedAABB.Area();

        // Cost of creating a new parent for this node and the new leaf
        float cost = 2.0f * combinedArea;

        // Minimum cost of pushing the leaf further down the tree
        float inheritanceCost = 2.0f * (combinedArea - area);

        // Cost of descending into child1
        float cost1;
        if (nodes[child1].IsLeaf()) {
            AABB aabb = leafAABB + nodes[child1].aabb;
            cost1 = aabb.Area() + inheritanceCost;
        } else {
            AABB aabb = leafAABB + nodes[child1].aabb;
            float oldArea = nodes[child1].aabb.Area();
            float newArea = aabb.Area();
            cost1 = (newArea - oldArea) + inheritanceCost;
        }

        // Cost of descending into child2
        float cost2;
        if (nodes[child2].IsLeaf()) {
            AABB aabb = leafAABB + nodes[child2].aabb;
            cost2 = aabb.Area() + inheritanceCost;
        } else {
            AABB aabb = leafAABB + nodes[child2].aabb;
            float oldArea = nodes[child2].aabb.Area();
            float newArea = aabb.Area();
            cost2 = newArea - oldArea + inheritanceCost;
        }

        // Descend according to the minimum cost.
        if (cost < cost1 && cost < cost2) {
            break;
        }

        // Descend
        if (cost1 < cost2) {
            index = child1;
        } else {
            index = child2;
        }
    }

    int32_t sibling = index;

    // Create a new parent.
    int32_t oldParent = nodes[sibling].parent;
    int32_t newParent = AllocNode();
    nodes[newParent].parent = oldParent;
    nodes[newParent].userData = nullptr;
    nodes[newParent].aabb = leafAABB + nodes[sibling].aabb;
    nodes[newParent].height = nodes[sibling].height + 1;

    if (oldParent != -1) {
        // The sibling was not the root.
        if (nodes[oldParent].child1 == sibling) {
            nodes[oldParent].child1 = newParent;
        } else {
            nodes[oldParent].child2 = newParent;
        }	
    } else {
        // The sibling was the root.
        root = newParent;
    }
    nodes[newParent].child1 = sibling;
    nodes[newParent].child2 = leaf;
    nodes[sibling].parent = newParent;
    nodes[leaf].parent = newParent;

    // Walk back up the tree fixing heights and AABBs
    index = nodes[leaf].parent;
    while (index != -1) {
        index = Balance(index);

        int32_t child1 = nodes[index].child1;
        int32_t child2 = nodes[index].child2;

        assert(child1 != -1);
        assert(child2 != -1);

        nodes[index].height = 1 + Max(nodes[child1].height, nodes[child2].height);
        nodes[index].aabb = nodes[child1].aabb + nodes[child2].aabb;

        index = nodes[index].parent;
    }

    //Validate();
}

void DynamicAABBTree::RemoveLeaf(int32_t leaf) {
    if (leaf == root) {
        root = -1;
        return;
    }

    int32_t parent = nodes[leaf].parent;
    int32_t grandParent = nodes[parent].parent;
    int32_t sibling;
    if (nodes[parent].child1 == leaf) {
        sibling = nodes[parent].child2;
    } else {
        sibling = nodes[parent].child1;
    }

    if (grandParent != -1) {
        // Destroy parent and connect sibling to grandParent.
        if (nodes[grandParent].child1 == parent) {
            nodes[grandParent].child1 = sibling;
        } else {
            nodes[grandParent].child2 = sibling;
        }
        nodes[sibling].parent = grandParent;
        FreeNode(parent);

        // Adjust ancestor bounds.
        int32_t index = grandParent;
        while (index != -1) {
            index = Balance(index);

            int32_t child1 = nodes[index].child1;
            int32_t child2 = nodes[index].child2;

            nodes[index].aabb = nodes[child1].aabb + nodes[child2].aabb;
            nodes[index].height = 1 + Max(nodes[child1].height, nodes[child2].height);

            index = nodes[index].parent;
        }
    } else {
        root = sibling;
        nodes[sibling].parent = -1;
        FreeNode(parent);
    }

    //Validate();
}

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
int32_t DynamicAABBTree::Balance(int32_t iA) {
    assert(iA != -1);

    Node *A = nodes + iA;
    if (A->IsLeaf() || A->height < 2) {
        return iA;
    }

    int32_t iB = A->child1;
    int32_t iC = A->child2;
    assert(0 <= iB && iB < nodeCapacity);
    assert(0 <= iC && iC < nodeCapacity);

    Node *B = nodes + iB;
    Node *C = nodes + iC;

    int32_t balance = C->height - B->height;

    // Rotate C up
    if (balance > 1) {
        int32_t iF = C->child1;
        int32_t iG = C->child2;
        Node *F = nodes + iF;
        Node *G = nodes + iG;
        assert(0 <= iF && iF < nodeCapacity);
        assert(0 <= iG && iG < nodeCapacity);

        // Swap A and C
        C->child1 = iA;
        C->parent = A->parent;
        A->parent = iC;

        // A's old parent should point to C
        if (C->parent != -1) {
            if (nodes[C->parent].child1 == iA) {
                nodes[C->parent].child1 = iC;
            } else {
                assert(nodes[C->parent].child2 == iA);
                nodes[C->parent].child2 = iC;
            }
        } else {
            root = iC;
        }

        // Rotate
        if (F->height > G->height) {
            C->child2 = iF;
            A->child2 = iG;
            G->parent = iA;
            A->aabb = B->aabb + G->aabb;
            C->aabb = A->aabb + F->aabb;

            A->height = 1 + Max(B->height, G->height);
            C->height = 1 + Max(A->height, F->height);
        } else {
            C->child2 = iG;
            A->child2 = iF;
            F->parent = iA;
            A->aabb = B->aabb + F->aabb;
            C->aabb = A->aabb + G->aabb;

            A->height = 1 + Max(B->height, F->height);
            C->height = 1 + Max(A->height, G->height);
        }

        return iC;
    }

    // Rotate B up
    if (balance < -1) {
        int32_t iD = B->child1;
        int32_t iE = B->child2;
        Node *D = nodes + iD;
        Node *E = nodes + iE;
        assert(0 <= iD && iD < nodeCapacity);
        assert(0 <= iE && iE < nodeCapacity);

        // Swap A and B
        B->child1 = iA;
        B->parent = A->parent;
        A->parent = iB;

        // A's old parent should point to B
        if (B->parent != -1) {
            if (nodes[B->parent].child1 == iA) {
                nodes[B->parent].child1 = iB;
            } else {
                assert(nodes[B->parent].child2 == iA);
                nodes[B->parent].child2 = iB;
            }
        } else {
            root = iB;
        }

        // Rotate
        if (D->height > E->height) {
            B->child2 = iD;
            A->child1 = iE;
            E->parent = iA;
            A->aabb = C->aabb + E->aabb;
            B->aabb = A->aabb + D->aabb;

            A->height = 1 + Max(C->height, E->height);
            B->height = 1 + Max(A->height, D->height);
        } else {
            B->child2 = iE;
            A->child1 = iD;
            D->parent = iA;
            A->aabb = C->aabb + D->aabb;
            B->aabb = A->aabb + E->aabb;

            A->height = 1 + Max(C->height, D->height);
            B->height = 1 + Max(A->height, E->height);
        }

        return iB;
    }

    return iA;
}

int32_t DynamicAABBTree::GetHeight() const {
    if (root == -1) {
        return 0;
    }

    return nodes[root].height;
}

float DynamicAABBTree::GetAreaRatio() const {
    if (root == -1) {
        return 0.0f;
    }

    const Node *rootNode = nodes + root;
    float rootArea = rootNode->aabb.Area();

    float totalArea = 0.0f;
    for (int32_t i = 0; i < nodeCapacity; ++i) {
        const Node *node = nodes + i;
        if (node->height < 0) {
            // Free node in pool
            continue;
        }

        totalArea += node->aabb.Area();
    }

    return totalArea / rootArea;
}

// Compute the height of a sub-tree.
int32_t DynamicAABBTree::ComputeHeight(int32_t nodeId) const {
    assert(0 <= nodeId && nodeId < nodeCapacity);
    Node *node = nodes + nodeId;

    if (node->IsLeaf()) {
        return 0;
    }

    int32_t height1 = ComputeHeight(node->child1);
    int32_t height2 = ComputeHeight(node->child2);
    return 1 + Max(height1, height2);
}

int32_t DynamicAABBTree::ComputeHeight() const {
    int32_t height = ComputeHeight(root);
    return height;
}

int32_t DynamicAABBTree::GetMaxBalance() const {
    int32_t maxBalance = 0;
    for (int32_t i = 0; i < nodeCapacity; ++i) {
        const Node *node = nodes + i;
        if (node->height <= 1) {
            continue;
        }

        assert(node->IsLeaf() == false);

        int32_t child1 = node->child1;
        int32_t child2 = node->child2;
        int32_t balance = Math::Abs(nodes[child2].height - nodes[child1].height);
        maxBalance = Max(maxBalance, balance);
    }

    return maxBalance;
}

#pragma optimize("", on)

void DynamicAABBTree::ValidateStructure(int32_t index) const {
    if (index == -1) {
        return;
    }

    if (index == root) {
        if (nodes[index].parent != -1) {
            BE_WARNLOG(L"DynamicAABBTree::ValidateStructure: root has parent\n");
        }
    }

    const Node *node = nodes + index;

    int32_t child1 = node->child1;
    int32_t child2 = node->child2;

    if (node->IsLeaf()) {
        if (child1 != -1 || child2 != -1 || node->height != 0) {
            BE_WARNLOG(L"DynamicAABBTree::ValidateStructure: invalid leaf\n");
        }
        return;
    }

    if (child1 < 0 || child1 >= nodeCapacity || nodes[child1].parent != index) {
        BE_WARNLOG(L"DynamicAABBTree::ValidateStructure: invalid node child1\n");
    }

    if (child2 < 0 || child2 >= nodeCapacity || nodes[child2].parent != index) {
        BE_WARNLOG(L"DynamicAABBTree::ValidateStructure: invalid node child2\n");
    }

    ValidateStructure(child1);
    ValidateStructure(child2);
}

void DynamicAABBTree::ValidateMetrics(int32_t index) const {
    if (index == -1) {
        return;
    }

    const Node *node = nodes + index;

    int32_t child1 = node->child1;
    int32_t child2 = node->child2;

    if (node->IsLeaf()) {
        if (child1 != -1 || child2 != -1 || node->height != 0) {
            BE_WARNLOG(L"DynamicAABBTree::ValidateMetrics: invalid leaf\n");
        }
        return;
    }

    if (child1 < 0 || child1 >= nodeCapacity) {
        BE_WARNLOG(L"DynamicAABBTree::ValidateMetrics: invalid node child1\n");
    }

    if (child2 < 0 || child2 >= nodeCapacity) {
        BE_WARNLOG(L"DynamicAABBTree::ValidateMetrics: invalid node child2\n");
    }

    int32_t height1 = nodes[child1].height;
    int32_t height2 = nodes[child2].height;
    int32_t height;
    height = 1 + Max(height1, height2);
    assert(node->height == height);

    AABB aabb = nodes[child1].aabb + nodes[child2].aabb;

    assert(aabb.b[0] == node->aabb.b[0]);
    assert(aabb.b[1] == node->aabb.b[1]);

    ValidateMetrics(child1);
    ValidateMetrics(child2);
}

void DynamicAABBTree::Validate() const {
    ValidateStructure(root);
    ValidateMetrics(root);

    int32_t freeCount = 0;
    int32_t freeIndex = freeList;
    while (freeIndex != -1) {
        assert(0 <= freeIndex && freeIndex < nodeCapacity);
        freeIndex = nodes[freeIndex].next;
        ++freeCount;
    }

    assert(GetHeight() == ComputeHeight());
    assert(nodeCount + freeCount == nodeCapacity);
}

void DynamicAABBTree::RebuildBottomUp() {
    if (nodeCount == 0) {
        return;
    }

    int32_t *nodeIndexes = (int32_t *)Mem_Alloc(nodeCount * sizeof(int32_t));
    int32_t count = 0;

    // Build array of leaves. Free the rest.
    for (int32_t i = 0; i < nodeCapacity; ++i) {
        if (nodes[i].height < 0) {
            // free node in pool
            continue;
        }

        if (nodes[i].IsLeaf()) {
            nodes[i].parent = -1;
            nodeIndexes[count] = i;
            ++count;
        } else {
            FreeNode(i);
        }
    }

    while (count > 1) {
        float minCost = FLT_MAX;
        int32_t iMin = -1, jMin = -1;
        for (int32_t i = 0; i < count; ++i) {
            AABB aabbi = nodes[nodeIndexes[i]].aabb;

            for (int32_t j = i + 1; j < count; ++j) {
                AABB aabbj = nodes[nodeIndexes[j]].aabb;
                AABB b = aabbi + aabbj;
                float cost = b.Area();
                if (cost < minCost) {
                    iMin = i;
                    jMin = j;
                    minCost = cost;
                }
            }
        }

        int32_t index1 = nodeIndexes[iMin];
        int32_t index2 = nodeIndexes[jMin];
        Node *child1 = nodes + index1;
        Node *child2 = nodes + index2;

        int32_t parentIndex = AllocNode();
        Node *parent = nodes + parentIndex;
        parent->child1 = index1;
        parent->child2 = index2;
        parent->height = 1 + Max(child1->height, child2->height);
        parent->aabb = child1->aabb + child2->aabb;
        parent->parent = -1;

        child1->parent = parentIndex;
        child2->parent = parentIndex;

        nodeIndexes[jMin] = nodeIndexes[count - 1];
        nodeIndexes[iMin] = parentIndex;
        count--;
    }

    root = nodeIndexes[0];
    Mem_Free(nodeIndexes);

    Validate();
}

#pragma optimize("", off)

BE_NAMESPACE_END
