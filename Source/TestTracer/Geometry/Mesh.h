#pragma once

#include "BlueshiftEngine.h"

struct Vert {
    BE1::Vec3                       xyz;
    BE1::Vec3                       normal;
    BE1::Vec2                       st;
};

class Material;

class SubMesh {
    friend class                    Model;

public:
    SubMesh(const SubMesh &) = delete;
    SubMesh &operator=(const SubMesh &) = delete;
    SubMesh() = default;
    ~SubMesh();

    void                            AllocSubMesh(int numVerts, int numIndexes);
    void                            FreeSubMesh();

    int                             numVerts = 0;
    Vert *                          verts = nullptr;
    int                             numIndexes = 0;
    int *                           indexes = nullptr;

                                    // Observer; owned by the scene or by the model's imported material list.
    Material *                      material = nullptr;

private:
    void                            ComputeSmoothNormals();
};
