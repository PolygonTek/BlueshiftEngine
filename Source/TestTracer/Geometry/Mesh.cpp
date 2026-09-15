#include "Precompiled.h"
#include "Geometry/Mesh.h"

SubMesh::~SubMesh() {
    FreeSubMesh();
}

void SubMesh::AllocSubMesh(int numVerts, int numIndexes) {
    FreeSubMesh();
    this->numVerts = numVerts;
    this->verts = new Vert[numVerts]{};
    this->numIndexes = numIndexes;
    this->indexes = new int[numIndexes];
    this->material = nullptr;
}

void SubMesh::FreeSubMesh() {
    numVerts = numIndexes = 0;
    material = nullptr;

    if (verts) {
        delete[] verts;
        verts = nullptr;
    }

    if (indexes) {
        delete[] indexes;
        indexes = nullptr;
    }
}

void SubMesh::ComputeSmoothNormals() {
    for (int i = 0; i < numVerts; i++) {
        verts[i].normal.SetFromScalar(0);
    }

    for (int i = 0; i < numIndexes; i += 3) {
        Vert &v0 = verts[indexes[i]];
        Vert &v1 = verts[indexes[i + 1]];
        Vert &v2 = verts[indexes[i + 2]];

        BE1::Vec3 n = (v1.xyz - v0.xyz).Cross(v2.xyz - v0.xyz);
        n.NormalizeFast();

        v0.normal += n;
        v1.normal += n;
        v2.normal += n;
    }

    for (int i = 0; i < numVerts; i++) {
        verts[i].normal.Normalize();
    }
}
