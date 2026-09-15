#pragma once

#include "Geometry/BVH.h"
#include <memory>
#include <vector>

class SubMesh;
class Material;

class Model {
public:
    Model &                         operator=(const Model &) = delete;

    int                             NumMeshes() const { return (int)meshList.size(); }
    const SubMesh *                 GetMesh(int index) const { return meshList[index].get(); }
    SubMesh *                       GetMesh(int index) { return meshList[index].get(); }
    const BE1::AABB &               GetAABB() const { return bvh->GetAABB(); }

    bool                            RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, HitTri *hit, float maxDistance = FLT_MAX) const;
    bool                            LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const;

    SubMesh *                       AllocSubMesh(int numVerts, int numIndexes);
    void                            FinishMeshes();

    void                            LoadFromObj(const char *filename);
    void                            BuildBVH();

private:
    // Imported materials remain owned even when a mesh receives an override.
    std::vector<std::unique_ptr<Material>> importedMaterials;
    std::vector<std::unique_ptr<SubMesh>> meshList;
    std::unique_ptr<BVH>            bvh;
};

BE_INLINE bool Model::RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, HitTri *hit, float maxDistance) const {
    return bvh->RayIntersection(start, dir, hit, maxDistance);
}

BE_INLINE bool Model::LineIntersection(const BE1::Vec3 &start, const BE1::Vec3 &end) const {
    return bvh->LineIntersection(start, end);
}
