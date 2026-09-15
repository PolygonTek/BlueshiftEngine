#include "Precompiled.h"
#include "Geometry/BVH.h"
#include "Geometry/Mesh.h"
#include "Geometry/Model.h"
#include <algorithm>
#include <memory>

static constexpr int MaxBVHDepth = 30;
static constexpr int MinTrianglesPerLeaf = 8;

struct BVHBuildContext {
    explicit BVHBuildContext(int numTris)
        : tris(new BVHBuildTri[numTris]), order(new int[numTris]), sorted(new int[numTris]), suffixBounds(new BE1::AABB[numTris]) {}

    // Only order is partitioned recursively; scratch storage is reused after each split.
    std::unique_ptr<BVHBuildTri[]> tris;
    std::unique_ptr<int[]> order;
    std::unique_ptr<int[]> sorted;
    std::unique_ptr<BE1::AABB[]> suffixBounds;
};

void BVH::Build(const Model *model) {
    Clear();
    this->model = model;

    int numTotalTris = 0;
    for (int i = 0; i < model->NumMeshes(); i++) {
        numTotalTris += model->GetMesh(i)->numIndexes / 3;
    }
    if (numTotalTris == 0) {
        return;
    }

    const int estimatedLeaves = (numTotalTris + MinTrianglesPerLeaf - 1) / MinTrianglesPerLeaf;
    nodeList.reserve(estimatedLeaves);
    leafList.reserve(estimatedLeaves);
    triangleList.reserve(numTotalTris);
    BVHBuildContext context(numTotalTris);

    BE1::AABB aabb;
    aabb.Clear();
    int tidx = 0;
    for (int i = 0; i < model->NumMeshes(); i++) {
        const SubMesh *mesh = model->GetMesh(i);
        const int numTris = mesh->numIndexes / 3;
        for (int j = 0; j < numTris; j++) {
            const BE1::Vec3 &v0 = mesh->verts[mesh->indexes[j * 3]].xyz;
            const BE1::Vec3 &v1 = mesh->verts[mesh->indexes[j * 3 + 1]].xyz;
            const BE1::Vec3 &v2 = mesh->verts[mesh->indexes[j * 3 + 2]].xyz;

            BVHBuildTri &tri = context.tris[tidx];
            tri.triIndex.meshIndex = i;
            tri.triIndex.triIndex = j;
            tri.center = (v0 + v1 + v2) / 3;
            tri.aabb.Clear();
            tri.aabb.AddPoint(v0);
            tri.aabb.AddPoint(v1);
            tri.aabb.AddPoint(v2);
            aabb.AddAABB(tri.aabb);
            context.order[tidx] = tidx;
            ++tidx;
        }
    }

    rootIndex = BuildRecursive(aabb, 0, numTotalTris, 1, context);
}

int BVH::BuildRecursive(const BE1::AABB &aabb, int firstTri, int numTris, int depth, BVHBuildContext &context) {
    if (numTris <= MinTrianglesPerLeaf || depth == MaxBVHDepth) {
        return MakeLeaf(aabb, firstTri, numTris, context);
    }

    // 가장 적은 cost 의 AABB split 정보를 구한다.
    BVHSplit split;
    if (!FindBestAABBSplit(aabb, firstTri, numTris, context, &split)) {
        return MakeLeaf(aabb, firstTri, numTris, context);
    }

    // split 후 cost 가 split 전 cost 보다 나아지는게 없다면 그만 쪼갠다.
    if (split.cost >= (float)numTris) {
        return MakeLeaf(aabb, firstTri, numTris, context);
    }

    const int nodeIndex = (int)nodeList.size();
    nodeList.push_back({aabb, split.axis, {0, 0}});

    // Recursive insertion may reallocate nodeList; retain indices rather than references.
    const int left = BuildRecursive(split.aabb[0], firstTri, split.numLeftTris, depth + 1, context);
    const int right = BuildRecursive(split.aabb[1], firstTri + split.numLeftTris, numTris - split.numLeftTris, depth + 1, context);
    nodeList[nodeIndex].child[0] = left;
    nodeList[nodeIndex].child[1] = right;
    return nodeIndex;
}

int BVH::MakeLeaf(const BE1::AABB &aabb, int firstTri, int numTris, const BVHBuildContext &context) {
    const int triangleOffset = (int)triangleList.size();
    for (int i = 0; i < numTris; i++) {
        triangleList.push_back(context.tris[context.order[firstTri + i]].triIndex);
    }
    leafList.push_back({aabb, numTris, triangleOffset});
    return -(int)leafList.size();
}

bool BVH::FindBestAABBSplit(const BE1::AABB &aabb, int firstTri, int numTris, BVHBuildContext &context, BVHSplit *split) const {
    split->axis = -1;
    split->cost = FLT_MAX;

    // 전체(parent) AABB 의 Surface Area
    const float parentSA = aabb.Area();
    if (parentSA <= 0.0f || !BE1::Math::IsFinite(parentSA)) {
        return false;
    }

    int *order = context.order.get() + firstTri;
    int *sorted = context.sorted.get();
    BE1::AABB *suffixBounds = context.suffixBounds.get();
    const BVHBuildTri *tris = context.tris.get();
    for (int axis = 0; axis < 3; axis++) {
        // Sort indices along each axis without copying triangle data.
        memcpy(sorted, order, sizeof(sorted[0]) * numTris);
        std::sort(sorted, sorted + numTris, [tris, axis](int lhs, int rhs) {
            const float a = tris[lhs].center[axis];
            const float b = tris[rhs].center[axis];
            return a == b ? lhs < rhs : a < b;
        });

        // axis 별로 뒤에서부터 AABB 합을 계산해서 테이블에 넣는다.
        suffixBounds[numTris - 1] = tris[sorted[numTris - 1]].aabb;
        for (int i = numTris - 2; i >= 0; i--) {
            suffixBounds[i] = suffixBounds[i + 1];
            suffixBounds[i].AddAABB(tris[sorted[i]].aabb);
        }

        BE1::AABB aabb1;
        aabb1.Clear();
        for (int i = 0; i < numTris - 1; i++) {
            aabb1.AddAABB(tris[sorted[i]].aabb);
            if (aabb1.IsCleared()) {
                continue;
            }
            const BE1::AABB &aabb2 = suffixBounds[i + 1];
            if (aabb2.IsCleared()) {
                continue;
            }

            const int n0 = i + 1;
            const int n1 = numTris - n0;
            // 양쪽 삼각형 AABB 합의 경계를 기준으로 최소 SAH cost 를 구한다.
            const float cost = (aabb1.Area() * n0 + aabb2.Area() * n1) / parentSA;
            if (cost < split->cost) {
                split->cost = cost;
                split->axis = axis;
                split->numLeftTris = n0;
                split->aabb[0] = aabb1;
                split->aabb[1] = aabb2;
            }
        }

        // Keep the winning permutation outside scratch storage before recursing.
        if (split->axis == axis) {
            memcpy(order, sorted, sizeof(order[0]) * numTris);
        }
    }
    return split->axis >= 0;
}

bool BVH::RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, HitTri *hit, float maxDistance) const {
    if (triangleList.empty() || maxDistance <= 0.0f) {
        return false;
    }
    const BE1::Ray ray(start, dir);
    float entry;
    if (!NodeAABB(rootIndex).IntersectRay(ray, &entry, nullptr) || entry >= maxDistance) {
        return false;
    }
    return RayIntersection(rootIndex, ray, maxDistance, hit);
}

bool BVH::RayIntersection(int nodeIndex, const BE1::Ray &ray, float &maxDistance, HitTri *hit) const {
    if (nodeIndex < 0) {
        const BVHLeaf &leaf = leafList[-(nodeIndex + 1)];
        const BVHTriIndex *closestTri = nullptr;
        BE1::Vec3 closestBarycentric;
        for (int i = 0; i < leaf.numTris; i++) {
            const BVHTriIndex &index = triangleList[leaf.firstTri + i];
            const SubMesh *mesh = model->GetMesh(index.meshIndex);
            const int tindex = index.triIndex * 3;
            const BE1::Vec3 &v0 = mesh->verts[mesh->indexes[tindex]].xyz;
            const BE1::Vec3 &v1 = mesh->verts[mesh->indexes[tindex + 1]].xyz;
            const BE1::Vec3 &v2 = mesh->verts[mesh->indexes[tindex + 2]].xyz;

            float distance;
            BE1::Vec3 barycentric;
            if (BE1::Triangle(v0, v1, v2).IntersectRay(ray, &distance, &barycentric, 0.0f) && distance < maxDistance) {
                maxDistance = distance;
                closestTri = &index;
                closestBarycentric = barycentric;
            }
        }
        if (!closestTri) {
            return false;
        }

        hit->t = maxDistance;
        hit->meshIndex = closestTri->meshIndex;
        const SubMesh *mesh = model->GetMesh(closestTri->meshIndex);
        const int tindex = closestTri->triIndex * 3;
        hit->index[0] = mesh->indexes[tindex];
        hit->index[1] = mesh->indexes[tindex + 1];
        hit->index[2] = mesh->indexes[tindex + 2];
        hit->u = closestBarycentric.x;
        hit->v = closestBarycentric.y;
        hit->w = closestBarycentric.z;
        return true;
    }

    const BVHNode &node = nodeList[nodeIndex];
    float entry[2];
    const bool intersects0 = NodeAABB(node.child[0]).IntersectRay(ray, &entry[0], nullptr) && entry[0] < maxDistance;
    const bool intersects1 = NodeAABB(node.child[1]).IntersectRay(ray, &entry[1], nullptr) && entry[1] < maxDistance;
    if (!intersects0) {
        return intersects1 && RayIntersection(node.child[1], ray, maxDistance, hit);
    }
    if (!intersects1) {
        return RayIntersection(node.child[0], ray, maxDistance, hit);
    }

    const int first = entry[1] < entry[0] ? 1 : 0;
    const int second = first ^ 1;
    bool found = RayIntersection(node.child[first], ray, maxDistance, hit);
    if (entry[second] < maxDistance && RayIntersection(node.child[second], ray, maxDistance, hit)) {
        found = true;
    }
    return found;
}

bool BVH::LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const {
    return !triangleList.empty() && LineIntersection(rootIndex, start, end);
}

bool BVH::LineIntersection(int nodeIndex, const BE1::Vec3 &start, const BE1::Vec3 &end) const {
    if (!NodeAABB(nodeIndex).IsIntersectLine(start, end)) {
        return false;
    }
    if (nodeIndex < 0) {
        const BVHLeaf &leaf = leafList[-(nodeIndex + 1)];
        for (int i = 0; i < leaf.numTris; i++) {
            const BVHTriIndex &index = triangleList[leaf.firstTri + i];
            const SubMesh *mesh = model->GetMesh(index.meshIndex);
            const int tindex = index.triIndex * 3;
            const BE1::Vec3 &v0 = mesh->verts[mesh->indexes[tindex]].xyz;
            const BE1::Vec3 &v1 = mesh->verts[mesh->indexes[tindex + 1]].xyz;
            const BE1::Vec3 &v2 = mesh->verts[mesh->indexes[tindex + 2]].xyz;
            if (BE1::Triangle(v0, v1, v2).IsIntersectLine(start, end)) {
                return true;
            }
        }
        return false;
    }

    // Any hit is sufficient for shadows; try the child along the segment direction first.
    const BVHNode &node = nodeList[nodeIndex];
    const int first = end[node.axis] < start[node.axis] ? 1 : 0;
    return LineIntersection(node.child[first], start, end) || LineIntersection(node.child[first ^ 1], start, end);
}
