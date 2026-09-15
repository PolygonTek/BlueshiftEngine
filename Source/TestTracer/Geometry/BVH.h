#pragma once

#include "BlueshiftEngine.h"
#include <vector>

struct BVHTriIndex {
    int                             meshIndex;
    int                             triIndex;
};

struct BVHBuildTri {
    BVHTriIndex                     triIndex;
    BE1::Vec3                       center;
    BE1::AABB                       aabb;
};

struct BVHNode {
    BE1::AABB                       bv;
    int                             axis;
    int                             child[2];
};

struct BVHLeaf {
    BE1::AABB                       bv;
    int                             numTris;
    int                             firstTri;
};

struct BVHSplit {
    int                             axis;
    float                           cost;
    BE1::AABB                       aabb[2];
    int                             numLeftTris;
};

struct BVHBuildContext;

struct HitTri {
    int                             meshIndex;
    int                             index[3];
    float                           u, v, w;
    float                           t;
};

class Model;

class BVH {
public:
    BVH() = default;
    BVH(const BVH &) = delete;
    BVH &operator=(const BVH &) = delete;

    void                            Clear();
    void                            Build(const Model *model);

    const BE1::AABB &               GetAABB() const { return triangleList.empty() ? BE1::AABB::empty : NodeAABB(rootIndex); }

                                    // Returns the closest hit strictly below maxDistance.
    bool                            RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, HitTri *hit, float maxDistance = FLT_MAX) const;
    bool                            LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const;

private:
    int                             BuildRecursive(const BE1::AABB &aabb, int firstTri, int numTris, int depth, BVHBuildContext &context);
    int                             MakeLeaf(const BE1::AABB &aabb, int firstTri, int numTris, const BVHBuildContext &context);
    bool                            FindBestAABBSplit(const BE1::AABB &aabb, int firstTri, int numTris, BVHBuildContext &context, BVHSplit *split) const;

    const BE1::AABB &               NodeAABB(int nodeIndex) const;
    bool                            RayIntersection(int nodeIndex, const BE1::Ray &ray, float &maxDistance, HitTri *hit) const;
    bool                            LineIntersection(int nodeIndex, const BE1::Vec3 &start, const BE1::Vec3 &end) const;

    std::vector<BVHNode>            nodeList;
    std::vector<BVHLeaf>            leafList;
    std::vector<BVHTriIndex>        triangleList;
    const Model *                   model = nullptr;
    int                             rootIndex = 0;
};

BE_INLINE void BVH::Clear() {
    nodeList.clear();
    leafList.clear();
    triangleList.clear();
    model = nullptr;
    rootIndex = 0;
}

BE_INLINE const BE1::AABB &BVH::NodeAABB(int nodeIndex) const {
    if (nodeIndex < 0) {
        return leafList[-(nodeIndex + 1)].bv;
    }
    return nodeList[nodeIndex].bv;
}
