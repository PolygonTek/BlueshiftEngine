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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Containers/HashTable.h"
#include "delabella/delabella.h"

#define PAR_OCTASPHERE_IMPLEMENTATION
#include "par_octasphere.h"

BE_NAMESPACE_BEGIN

void Mesh::CreateDefaultBox() {
    Purge();

    MeshSurf *surf = AllocSurface(24, 36);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    constexpr float extent = MeterToUnit(0.1f);

    VertexGenericLit *v = surf->subMesh->verts;
    // bottom face
    v[0].SetPosition(-extent, -extent, 0.0f); v[0].SetTexCoord(0.0f, 1.0f); v[0].SetNormal(0.0f, 0.0f, -1.0f); v[0].SetColor(0xffffffff);
    v[1].SetPosition(-extent, +extent, 0.0f); v[1].SetTexCoord(1.0f, 1.0f); v[1].SetNormal(0.0f, 0.0f, -1.0f); v[1].SetColor(0xffffffff);
    v[2].SetPosition(+extent, +extent, 0.0f); v[2].SetTexCoord(1.0f, 0.0f); v[2].SetNormal(0.0f, 0.0f, -1.0f); v[2].SetColor(0xffffffff);
    v[3].SetPosition(+extent, -extent, 0.0f); v[3].SetTexCoord(0.0f, 0.0f); v[3].SetNormal(0.0f, 0.0f, -1.0f); v[3].SetColor(0xffffffff);
    
    // top face
    v[4].SetPosition(+extent, -extent, extent * 2); v[4].SetTexCoord(0.0f, 1.0f); v[4].SetNormal(0.0f, 0.0f,  1.0f); v[4].SetColor(0xffffffff);
    v[5].SetPosition(+extent, +extent, extent * 2); v[5].SetTexCoord(1.0f, 1.0f); v[5].SetNormal(0.0f, 0.0f,  1.0f); v[5].SetColor(0xffffffff);
    v[6].SetPosition(-extent, +extent, extent * 2); v[6].SetTexCoord(1.0f, 0.0f); v[6].SetNormal(0.0f, 0.0f,  1.0f); v[6].SetColor(0xffffffff);
    v[7].SetPosition(-extent, -extent, extent * 2); v[7].SetTexCoord(0.0f, 0.0f); v[7].SetNormal(0.0f, 0.0f,  1.0f); v[7].SetColor(0xffffffff);
    
    // front face
    v[8].SetPosition(+extent, -extent, 0.0f); v[8].SetTexCoord(0.0f, 1.0f); v[8].SetNormal(1.0f, 0.0f, 0.0f); v[8].SetColor(0xffffffff);
    v[9].SetPosition(+extent, +extent, 0.0f); v[9].SetTexCoord(1.0f, 1.0f); v[9].SetNormal(1.0f, 0.0f, 0.0f); v[9].SetColor(0xffffffff);
    v[10].SetPosition(+extent, +extent, extent * 2); v[10].SetTexCoord(1.0f, 0.0f); v[10].SetNormal(1.0f, 0.0f, 0.0f); v[10].SetColor(0xffffffff);
    v[11].SetPosition(+extent, -extent, extent * 2); v[11].SetTexCoord(0.0f, 0.0f); v[11].SetNormal(1.0f, 0.0f, 0.0f); v[11].SetColor(0xffffffff);
    
    // right face
    v[12].SetPosition(+extent, +extent,  0.0f); v[12].SetTexCoord(0.0f, 1.0f); v[12].SetNormal(0.0f, 1.0f, 0.0f); v[12].SetColor(0xffffffff);
    v[13].SetPosition(-extent, +extent,  0.0f); v[13].SetTexCoord(1.0f, 1.0f); v[13].SetNormal(0.0f, 1.0f, 0.0f); v[13].SetColor(0xffffffff);
    v[14].SetPosition(-extent, +extent, extent * 2); v[14].SetTexCoord(1.0f, 0.0f); v[14].SetNormal(0.0f, 1.0f, 0.0f); v[14].SetColor(0xffffffff);
    v[15].SetPosition(+extent, +extent, extent * 2); v[15].SetTexCoord(0.0f, 0.0f); v[15].SetNormal(0.0f, 1.0f, 0.0f); v[15].SetColor(0xffffffff);
    
    // back face
    v[16].SetPosition(-extent, +extent,  0.0f); v[16].SetTexCoord(0.0f, 1.0f); v[16].SetNormal(-1.0f, 0.0f, 0.0f); v[16].SetColor(0xffffffff);
    v[17].SetPosition(-extent, -extent,  0.0f); v[17].SetTexCoord(1.0f, 1.0f); v[17].SetNormal(-1.0f, 0.0f, 0.0f); v[17].SetColor(0xffffffff);
    v[18].SetPosition(-extent, -extent, extent * 2); v[18].SetTexCoord(1.0f, 0.0f); v[18].SetNormal(-1.0f, 0.0f, 0.0f); v[18].SetColor(0xffffffff);
    v[19].SetPosition(-extent, +extent, extent * 2); v[19].SetTexCoord(0.0f, 0.0f); v[19].SetNormal(-1.0f, 0.0f, 0.0f); v[19].SetColor(0xffffffff);
    
    // left face
    v[20].SetPosition(-extent, -extent,  0.0f); v[20].SetTexCoord(0.0f, 1.0f); v[20].SetNormal(0.0f, -1.0f, 0.0f); v[20].SetColor(0xffffffff);
    v[21].SetPosition(+extent, -extent,  0.0f); v[21].SetTexCoord(1.0f, 1.0f); v[21].SetNormal(0.0f, -1.0f, 0.0f); v[21].SetColor(0xffffffff);
    v[22].SetPosition(+extent, -extent, extent * 2); v[22].SetTexCoord(1.0f, 0.0f); v[22].SetNormal(0.0f, -1.0f, 0.0f); v[22].SetColor(0xffffffff);
    v[23].SetPosition(-extent, -extent, extent * 2); v[23].SetTexCoord(0.0f, 0.0f); v[23].SetNormal(0.0f, -1.0f, 0.0f); v[23].SetColor(0xffffffff);
        
    VertIndex *idx = surf->subMesh->indexes;
    idx[0] = 0;     idx[1] = 1;     idx[2] = 2;
    idx[3] = 2;     idx[4] = 3;     idx[5] = 0;
    idx[6] = 4;     idx[7] = 5;     idx[8] = 6;
    idx[9] = 6;     idx[10] = 7;    idx[11] = 4;
    idx[12] = 8;    idx[13] = 9;    idx[14] = 10;
    idx[15] = 10;   idx[16] = 11;   idx[17] = 8;
    idx[18] = 12;   idx[19] = 13;   idx[20] = 14;
    idx[21] = 14;   idx[22] = 15;   idx[23] = 12;
    idx[24] = 16;   idx[25] = 17;   idx[26] = 18;
    idx[27] = 18;   idx[28] = 19;   idx[29] = 16;
    idx[30] = 20;   idx[31] = 21;   idx[32] = 22;
    idx[33] = 22;   idx[34] = 23;   idx[35] = 20;
    
    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents | FinishFlag::UseUnsmoothedTangents);
}

void Mesh::CreatePlane(const Vec3 &origin, const Mat3 &axis, float size, int numSegments) {
    assert(numSegments > 0);

    Purge();

    int numVerts = (numSegments + 1) * (numSegments + 1);
    int numIndexes = 6 * numSegments * numSegments;

    MeshSurf *surf = AllocSurface(numVerts, numIndexes);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    VertexGenericLit *v = surf->subMesh->verts;

    for (int i = 0; i < numSegments + 1; i++) { // y
        for (int j = 0; j < numSegments + 1; j++) { // x
            float y = size * ((float)i / numSegments - 0.5f);
            float x = size * ((float)j / numSegments - 0.5f);
            float s = (float)i / numSegments;
            float t = (float)j / numSegments;

            v->SetPosition(x, y, 0);
            v->SetNormal(0, 0, 1);
            v->SetTangent(0, 1, 0);
            v->SetBiTangent(1, 0, 0);
            v->SetTexCoord(s, t);
            v->SetColor(0xffffffff);
            v++;
        }
    }

    VertIndex *idx = surf->subMesh->indexes;

    for (int i = 0; i < numSegments; i++) { // y
        int a = (numSegments + 1) * i;

        for (int j = 0; j < numSegments; j++) { // x
            int b = a + j;
            int c = b + numSegments + 1;

            *idx++ = b;
            *idx++ = b + 1;
            *idx++ = c;
            
            *idx++ = c;
            *idx++ = b + 1;
            *idx++ = c + 1;
        }
    }
    
    FinishSurfaces(FinishFlag::ComputeAABB);

    if (!axis.IsIdentity() || !origin.IsZero()) {
        TransformVerts(axis, Vec3::one, origin);
    }
}

void Mesh::CreateBox(const Vec3 &origin, const Mat3 &axis, const Vec3 &extents) {
    Purge();

    MeshSurf *surf = AllocSurface(24, 36);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    VertexGenericLit *v = surf->subMesh->verts;
    // bottom face
    v[0].SetPosition(-extents[0], -extents[1], -extents[2]); v[0].SetTexCoord(0.0f, 1.0f); v[0].SetNormal(0.0f, 0.0f, -1.0f); v[0].SetColor(0xffffffff);
    v[1].SetPosition(-extents[0],  extents[1], -extents[2]); v[1].SetTexCoord(1.0f, 1.0f); v[1].SetNormal(0.0f, 0.0f, -1.0f); v[1].SetColor(0xffffffff);
    v[2].SetPosition( extents[0],  extents[1], -extents[2]); v[2].SetTexCoord(1.0f, 0.0f); v[2].SetNormal(0.0f, 0.0f, -1.0f); v[2].SetColor(0xffffffff);
    v[3].SetPosition( extents[0], -extents[1], -extents[2]); v[3].SetTexCoord(0.0f, 0.0f); v[3].SetNormal(0.0f, 0.0f, -1.0f); v[3].SetColor(0xffffffff);
    
    // top face
    v[4].SetPosition( extents[0], -extents[1],  extents[2]); v[4].SetTexCoord(0.0f, 1.0f); v[4].SetNormal(0.0f, 0.0f,  1.0f); v[4].SetColor(0xffffffff);
    v[5].SetPosition( extents[0],  extents[1],  extents[2]); v[5].SetTexCoord(1.0f, 1.0f); v[5].SetNormal(0.0f, 0.0f,  1.0f); v[5].SetColor(0xffffffff);
    v[6].SetPosition(-extents[0],  extents[1],  extents[2]); v[6].SetTexCoord(1.0f, 0.0f); v[6].SetNormal(0.0f, 0.0f,  1.0f); v[6].SetColor(0xffffffff);
    v[7].SetPosition(-extents[0], -extents[1],  extents[2]); v[7].SetTexCoord(0.0f, 0.0f); v[7].SetNormal(0.0f, 0.0f,  1.0f); v[7].SetColor(0xffffffff);

    // front face
    v[8].SetPosition( extents[0], -extents[1], -extents[2]); v[8].SetTexCoord(0.0f, 1.0f); v[8].SetNormal(1.0f, 0.0f, 0.0f); v[8].SetColor(0xffffffff);
    v[9].SetPosition( extents[0],  extents[1], -extents[2]); v[9].SetTexCoord(1.0f, 1.0f); v[9].SetNormal(1.0f, 0.0f, 0.0f); v[9].SetColor(0xffffffff);
    v[10].SetPosition(extents[0],  extents[1],  extents[2]); v[10].SetTexCoord(1.0f, 0.0f); v[10].SetNormal(1.0f, 0.0f, 0.0f); v[10].SetColor(0xffffffff);
    v[11].SetPosition(extents[0], -extents[1],  extents[2]); v[11].SetTexCoord(0.0f, 0.0f); v[11].SetNormal(1.0f, 0.0f, 0.0f); v[11].SetColor(0xffffffff);
    
    // right face
    v[12].SetPosition( extents[0], extents[1], -extents[2]); v[12].SetTexCoord(0.0f, 1.0f); v[12].SetNormal(0.0f, 1.0f, 0.0f); v[12].SetColor(0xffffffff);
    v[13].SetPosition(-extents[0], extents[1], -extents[2]); v[13].SetTexCoord(1.0f, 1.0f); v[13].SetNormal(0.0f, 1.0f, 0.0f); v[13].SetColor(0xffffffff);
    v[14].SetPosition(-extents[0], extents[1],  extents[2]); v[14].SetTexCoord(1.0f, 0.0f); v[14].SetNormal(0.0f, 1.0f, 0.0f); v[14].SetColor(0xffffffff);
    v[15].SetPosition( extents[0], extents[1],  extents[2]); v[15].SetTexCoord(0.0f, 0.0f); v[15].SetNormal(0.0f, 1.0f, 0.0f); v[15].SetColor(0xffffffff);
    
    // back face
    v[16].SetPosition(-extents[0],  extents[1], -extents[2]); v[16].SetTexCoord(0.0f, 1.0f); v[16].SetNormal(-1.0f, 0.0f, 0.0f); v[16].SetColor(0xffffffff);
    v[17].SetPosition(-extents[0], -extents[1], -extents[2]); v[17].SetTexCoord(1.0f, 1.0f); v[17].SetNormal(-1.0f, 0.0f, 0.0f); v[17].SetColor(0xffffffff);
    v[18].SetPosition(-extents[0], -extents[1],  extents[2]); v[18].SetTexCoord(1.0f, 0.0f); v[18].SetNormal(-1.0f, 0.0f, 0.0f); v[18].SetColor(0xffffffff);
    v[19].SetPosition(-extents[0],  extents[1],  extents[2]); v[19].SetTexCoord(0.0f, 0.0f); v[19].SetNormal(-1.0f, 0.0f, 0.0f); v[19].SetColor(0xffffffff);
    
    // left face
    v[20].SetPosition(-extents[0], -extents[1], -extents[2]); v[20].SetTexCoord(0.0f, 1.0f); v[20].SetNormal(0.0f, -1.0f, 0.0f); v[20].SetColor(0xffffffff);
    v[21].SetPosition( extents[0], -extents[1], -extents[2]); v[21].SetTexCoord(1.0f, 1.0f); v[21].SetNormal(0.0f, -1.0f, 0.0f); v[21].SetColor(0xffffffff);
    v[22].SetPosition( extents[0], -extents[1],  extents[2]); v[22].SetTexCoord(1.0f, 0.0f); v[22].SetNormal(0.0f, -1.0f, 0.0f); v[22].SetColor(0xffffffff);
    v[23].SetPosition(-extents[0], -extents[1],  extents[2]); v[23].SetTexCoord(0.0f, 0.0f); v[23].SetNormal(0.0f, -1.0f, 0.0f); v[23].SetColor(0xffffffff);
        
    VertIndex *idx = surf->subMesh->indexes;
    idx[0] = 0;     idx[1] = 1;     idx[2] = 2;
    idx[3] = 2;     idx[4] = 3;     idx[5] = 0;
    idx[6] = 4;     idx[7] = 5;     idx[8] = 6;
    idx[9] = 6;     idx[10] = 7;    idx[11] = 4;
    idx[12] = 8;    idx[13] = 9;    idx[14] = 10;
    idx[15] = 10;   idx[16] = 11;   idx[17] = 8;
    idx[18] = 12;   idx[19] = 13;   idx[20] = 14;
    idx[21] = 14;   idx[22] = 15;   idx[23] = 12;
    idx[24] = 16;   idx[25] = 17;   idx[26] = 18;
    idx[27] = 18;   idx[28] = 19;   idx[29] = 16;
    idx[30] = 20;   idx[31] = 21;   idx[32] = 22;
    idx[33] = 22;   idx[34] = 23;   idx[35] = 20;
    
    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents | FinishFlag::UseUnsmoothedTangents);

    if (!axis.IsIdentity() || !origin.IsZero()) {
        TransformVerts(axis, Vec3::one, origin);
    }
}

void Mesh::CreateSphere(const Vec3 &origin, const Mat3 &axis, float radius, int numSegments) {
    CreateCapsule(origin, axis, radius, 0, numSegments);
}

void Mesh::CreateGeosphere(const Vec3 &origin, float radius, int numSubdivisions) {
    assert(numSubdivisions >= 0);
#if 1
    CreateRoundedBox(origin, Mat3::identity, Vec3::zero, radius, numSubdivisions);
#else
    static constexpr float x = 0.525731112119133606f;
    static constexpr float z = 0.850650808352039932f;
    static constexpr Vec3 icosa_verts[12] = {
        Vec3(-x, 0, z), Vec3(x, 0, z), Vec3(-x, 0, -z), Vec3(x, 0, -z),
        Vec3(0, z, x), Vec3(0, z, -x), Vec3(0, -z, x), Vec3(0, -z, -x),
        Vec3(z, x, 0), Vec3(-z, x, 0), Vec3(z, -x, 0), Vec3(-z, -x, 0)
    };
    static constexpr VertIndex icosa_tris[20][3] = {
        { 0, 1, 4 }, { 0, 4, 9 }, { 9, 4, 5 }, { 4, 8, 5 }, { 4, 1, 8 },
        { 8, 1, 10 }, { 8, 10, 3 }, { 5, 8, 3 }, { 5, 3, 2 }, { 2, 3, 7 },
        { 7, 3, 10 }, { 7, 10, 6 }, { 7, 6, 11 }, { 11, 6, 0 }, { 0, 6, 1 },
        { 6, 10, 1 }, { 9, 11, 0 }, { 9, 2, 11 }, { 9, 5, 2 }, { 7, 11, 2 }
    };

    Purge();

    // TODO: subdivision

    MeshSurf *surf = AllocSurface(12, 60);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    VertexGenericLit *v = surf->subMesh->verts;
    for (int i = 0; i < COUNT_OF(icosa_verts); i++) {
        Vec3 n = icosa_verts[i];
        n.Normalize();
        float theta = Math::ATan(n.y, n.x);
        float phi = Math::ACos(n.z);
        float s = theta * Math::InvPi;
        float t = phi * Math::InvPi;

        v->SetPosition(icosa_verts[i] * radius);
        v->SetNormal(n);
        v->SetTexCoord(s, t);
        v->SetColor(0xffffffff);
        v++;
    }

    VertIndex *idx = surf->subMesh->indexes;
    for (int i = 0; i < COUNT_OF(icosa_tris); i++) {
        for (int j = 0; j < 3; j++) {
            *idx++ = icosa_tris[i][j];
        }
    }

    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);

    if (!origin.IsZero()) {
        TransformVerts(Mat3::identity, Vec3::one, origin);
    }
#endif
}

void Mesh::CreateRoundedBox(const Vec3 &origin, const Mat3 &axis, const Vec3 extents, float radius, int numSubdivisions) {
    assert(numSubdivisions >= 0);

    par_octasphere_config cfg;
    cfg.corner_radius = radius;
    cfg.width = (extents[0] + radius) * 2;
    cfg.height = (extents[1] + radius) * 2;
    cfg.depth = (extents[2] + radius) * 2;
    cfg.num_subdivisions = numSubdivisions;
    cfg.uv_mode = PAR_OCTASPHERE_UV_LATLONG;
    cfg.normals_mode = PAR_OCTASPHERE_NORMALS_SMOOTH;

    uint32_t num_indices;
    uint32_t num_vertices;
    par_octasphere_get_counts(&cfg, &num_indices, &num_vertices);

    MeshSurf *surf = AllocSurface(num_vertices, num_indices);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    par_octasphere_mesh mesh;
    mesh.positions = (float *)Mem_Alloc(sizeof(float) * 3 * num_vertices);
    mesh.normals = (float *)Mem_Alloc(sizeof(float) * 3 * num_vertices);
    mesh.texcoords = (float *)Mem_Alloc(sizeof(float) * 2 * num_vertices);
    mesh.indices = (uint16_t *)Mem_Alloc(sizeof(uint16_t) * num_indices);

    par_octasphere_populate(&cfg, &mesh);

    VertexGenericLit *vptr = surf->subMesh->verts;
    for (int i = 0; i < surf->subMesh->numVerts; i++) {
        vptr->SetPosition(mesh.positions[i * 3 + 0], mesh.positions[i * 3 + 1], mesh.positions[i * 3 + 2]);
        vptr->SetNormal(mesh.normals[i * 3 + 0], mesh.normals[i * 3 + 1], mesh.normals[i * 3 + 2]);
        vptr->SetTexCoord(mesh.texcoords[i * 2 + 0], mesh.texcoords[i * 2 + 1]);
        vptr->SetColor(0xffffffff);
        vptr++;
    }

    VertIndex *iptr = surf->subMesh->indexes;
    for (int i = 0; i < surf->subMesh->numIndexes; i++) {
        *iptr++ = (VertIndex)mesh.indices[i];
    }

    Mem_Free(mesh.positions);
    Mem_Free(mesh.normals);
    Mem_Free(mesh.texcoords);
    Mem_Free(mesh.indices);

    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);

    if (!axis.IsIdentity() || !origin.IsZero()) {
        TransformVerts(axis, Vec3::one, origin);
    }
}

void Mesh::CreateCylinder(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments) {
    assert(numSegments >= 4);

    Purge();

    Vec3 n[2] = { Vec3(0, 0, 1), Vec3(0, 0, -1) };
    float half_height = height * 0.5f;

    int numSideSegments = numSegments + 1;
    int numVerts = 2 + numSegments * 2 + numSideSegments * 2;
    int numIndexes = numSegments * 3 * 2 + numSideSegments * 3 * 2;

    MeshSurf *surf = AllocSurface(numVerts, numIndexes);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    VertexGenericLit *v = surf->subMesh->verts;

    // top cap, bottom cap verts
    for (int i = 0; i < 2; i++) {
        v->SetPosition(n[i] * half_height);
        v->SetNormal(n[i]);
        v->SetTexCoord(0.5f, 0.5f);
        v->SetColor(0xffffffff);
        v++;

        for (int j = 0; j < numSegments; j++) {
            float theta = Math::TwoPi * j / numSegments; // 0 ~ 2pi
            float st, ct;
            Math::SinCos(theta, st, ct);
            float x = ct * radius;
            float y = st * radius;
            float s = st * 0.5f + 0.5f;
            float t = ct * 0.5f + 0.5f;

            v->SetPosition(n[i] * half_height + Vec3(x, y, 0));
            v->SetNormal(n[i]);
            v->SetTexCoord(s, t);
            v->SetColor(0xffffffff);
            v++;
        }
    }

    // sides verts
    for (int i = 0; i < numSideSegments; i++) {
        float theta = Math::TwoPi * i / (numSideSegments - 1); // 0 ~ 2pi
        float st, ct;
        Math::SinCos(theta + Math::HalfPi, st, ct);
        float x = ct * radius;
        float y = st * radius;
        float s = theta * Math::InvPi;
        Vec3 n = Vec3(ct, st, 0);

        v->SetPosition(x, y, half_height);
        v->SetNormal(n);
        v->SetTexCoord(s, 0.0f);
        v->SetColor(0xffffffff);
        v++;

        v->SetPosition(x, y, -half_height);
        v->SetNormal(n);
        v->SetTexCoord(s, 1.0f);
        v->SetColor(0xffffffff);
        v++;
    }

    VertIndex *idx = surf->subMesh->indexes;

    // top cap indexes
    for (int j = 0; j < numSegments; j++) {
        *idx++ = 0;
        *idx++ = 1 + j;
        *idx++ = 1 + (j + 1) % numSegments;
    }

    int offset = numSegments + 1;

    // bottom cap indexes
    for (int j = 0; j < numSegments; j++) {
        *idx++ = offset;
        *idx++ = offset + 1 + (j + 1) % numSegments;
        *idx++ = offset + 1 + j;
    }

    offset += numSegments + 1;

    // sides indexes
    for (int i = 0; i < numSideSegments; i++) {
        int a = i * 2;
        int b = ((i + 1) % numSideSegments) * 2;

        *idx++ = offset + a;
        *idx++ = offset + a + 1;
        *idx++ = offset + b;

        *idx++ = offset + b;
        *idx++ = offset + a + 1;
        *idx++ = offset + b + 1;
    }

    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);

    if (!axis.IsIdentity() || !origin.IsZero()) {
        TransformVerts(axis, Vec3::one, origin);
    }
}

void Mesh::CreateCapsule(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments) {
    assert(numSegments >= 4);
    assert(numSegments % 4 == 0);

    Purge();

    float half_height = height * 0.5f;
    float invR = 1.0f / radius;
    float real_height = 2.0f * radius + height;
    
    int numLat = numSegments / 4 + 1;   // hemisphere latitude verts
    int numLng = numSegments + 1;       // hemisphere longitude verts

    int numVerts = numLat * 2 * numLng;
    int numIndexes = (6 * (numLat * 2 - 1) * (numLng - 1)) - (2 * 3 * (numLng - 1));

    MeshSurf *surf = AllocSurface(numVerts, numIndexes);
    surf->materialIndex = 0;
    surfaces.Append(surf);

    VertexGenericLit *v = surf->subMesh->verts;

    for (int i = 0; i < numLat; i++) {
        float phi = Math::HalfPi * i / (numLat - 1); // 0 ~ pi/2
        float sp, cp;
        Math::SinCos(phi, sp, cp);
        float z = cp * radius;
        float zr = sp * radius;
        float t = 1.0f - (z + radius + height) / real_height;

        for (int j = 0; j < numLng; j++) {
            float theta = Math::TwoPi * j / (numLng - 1); // 0 ~ 2pi
            float st, ct;
            Math::SinCos(theta - Math::HalfPi, st, ct);
            float x = ct * zr;
            float y = st * zr;
            float s = theta * Math::InvPi;

            v->SetPosition(x, y, z + half_height);
            v->SetNormal(x * invR, y * invR, cp);
            v->SetTangent(st * cp, ct * sp, st);
            v->SetBiTangent(v->GetNormal().Cross(v->GetTangent()));
            v->SetTexCoord(s, t);
            v->SetColor(0xffffffff);
            v++;
        }
    }

    for (int i = 0; i < numLat; i++) {
        float phi = Math::HalfPi + Math::HalfPi * i / (numLat - 1); // pi/2 ~ pi
        float sp, cp;
        Math::SinCos(phi, sp, cp);
        float z = cp * radius;
        float zr = sp * radius;
        float t = 1.0f - (z + radius) / real_height;

        for (int j = 0; j < numLng; j++) {
            float theta = Math::TwoPi * j / (numLng - 1); // 0 ~ 2pi
            float st, ct;
            Math::SinCos(theta - Math::HalfPi, st, ct);
            float x = ct * zr;
            float y = st * zr;
            float s = theta * Math::InvPi;

            v->SetPosition(x, y, z - half_height);
            v->SetNormal(x * invR, y * invR, cp);
            v->SetTangent(st * cp, ct * sp, st);
            v->SetBiTangent(v->GetNormal().Cross(v->GetTangent()));
            v->SetTexCoord(s, t);
            v->SetColor(0xffffffff);
            v++;
        }
    }

    VertIndex *idx = surf->subMesh->indexes;

    for (int i = 0; i < numLat * 2 - 1; i++) {
        int a = numLng * i;

        for (int j = 0; j < numLng - 1; j++) {
            if (i > 0) {
                int b = a + j;
                *idx++ = a + (j + 1) % numLng;
                *idx++ = b;
                *idx++ = b + numLng;
            }

            if (i < numLat * 2 - 2) {
                int b = a + (j + 1) % numLng;
                *idx++ = b;
                *idx++ = a + numLng + j;
                *idx++ = numLng + b;
            }
        }
    }

    FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);

    if (!axis.IsIdentity() || !origin.IsZero()) {
        TransformVerts(axis, Vec3::one, origin);
    }
}

bool Mesh::TrySliceMesh(const Mesh &srcMesh, const Plane &slicePlane, bool generateCap, float capTextureScale, bool generateAboveMesh, Mesh *outBelowMesh, Mesh *outAboveMesh) {
    assert(outBelowMesh);

    if (generateAboveMesh) {
        assert(outAboveMesh);
    }

    // If the entire mesh is above the plane, the operation is skipped.
    int planeSide = srcMesh.aabb.PlaneSide(slicePlane);
    if (planeSide == Plane::Side::Back || planeSide == Plane::Side::Front) {
        return false;
    }

    for (int surfaceIndex = 0; surfaceIndex < srcMesh.surfaces.Count(); surfaceIndex++) {
        const MeshSurf *srcSurf = srcMesh.surfaces[surfaceIndex];

        int planeSide = srcSurf->subMesh->aabb.PlaneSide(slicePlane);
        if (planeSide == Plane::Side::Back) {
            // SubMesh is totally below the plane. just copy it from the source mesh.
            MeshSurf* surf = outBelowMesh->AllocSurface(srcSurf->subMesh->numVerts, srcSurf->subMesh->numIndexes);
            surf->materialIndex = srcSurf->materialIndex;
            outBelowMesh->surfaces.Append(surf);
            surf->subMesh->CopyFrom(srcSurf->subMesh);
            continue;
        }

        if (planeSide == Plane::Side::Front) {
            // SubMesh totally is above the plane.
            if (generateAboveMesh) {
                MeshSurf *surf = outAboveMesh->AllocSurface(srcSurf->subMesh->numVerts, srcSurf->subMesh->numIndexes);
                surf->materialIndex = srcSurf->materialIndex;
                outAboveMesh->surfaces.Append(surf);
                surf->subMesh->CopyFrom(srcSurf->subMesh);
            }
            continue;
        }

        const VertexGenericLit *srcVerts = srcSurf->subMesh->verts;
        const VertIndex *srcIndexes = srcSurf->subMesh->indexes;
        int numSrcVerts = srcSurf->subMesh->NumVerts();
        int numSrcIndexes = srcSurf->subMesh->NumIndexes();

        Array<VertexGenericLit> tempBelowVerts;
        Array<VertexGenericLit> tempBelowCapVerts;
        Array<VertexGenericLit> tempAboveVerts;
        Array<VertexGenericLit> tempAboveCapVerts;
        Array<VertIndex> tempBelowIndexes;
        Array<VertIndex> tempBelowCapIndexes;
        Array<VertIndex> tempAboveIndexes;
        Array<VertIndex> tempAboveCapIndexes;

        // FIXME
        tempBelowVerts.Reserve(numSrcVerts * 2);
        tempAboveVerts.Reserve(numSrcVerts * 2);
        tempBelowIndexes.Reserve(numSrcIndexes * 2);
        tempAboveIndexes.Reserve(numSrcIndexes * 2);

        // source vertex index -> sliced vertex index
        HashTable<int, int> srcToBelowVertIndex;
        HashTable<int, int> srcToAboveVertIndex;

        Array<float> vertexDistances;
        vertexDistances.SetCount(numSrcVerts);

        for (int srcVertIndex = 0; srcVertIndex < numSrcVerts; srcVertIndex++) {
            // Distance of each vertex from slice plane.
            vertexDistances[srcVertIndex] = slicePlane.Distance(srcVerts[srcVertIndex].xyz);

            if (vertexDistances[srcVertIndex] <= 0.0f) {
                int slicedVertIndex = tempBelowVerts.Append(srcVerts[srcVertIndex]);

                srcToBelowVertIndex.Set(srcVertIndex, slicedVertIndex);
            } else {
                if (generateAboveMesh) {
                    int otherVertIndex = tempAboveVerts.Append(srcVerts[srcVertIndex]);

                    srcToAboveVertIndex.Set(srcVertIndex, otherVertIndex);
                }
            }
        }

        Array<Vec3> clippedVerts;
        
        int maxClippedVerts = 0;
        for (int baseIndex = 0; baseIndex < numSrcIndexes; baseIndex += 3) {
            int srcIndex[3];
            srcIndex[0] = srcIndexes[baseIndex + 0];
            srcIndex[1] = srcIndexes[baseIndex + 1];
            srcIndex[2] = srcIndexes[baseIndex + 2];
            int belowIndex[3] = { -1, -1, -1 };

            srcToBelowVertIndex.Get(srcIndex[0], &belowIndex[0]);
            srcToBelowVertIndex.Get(srcIndex[1], &belowIndex[1]);
            srcToBelowVertIndex.Get(srcIndex[2], &belowIndex[2]);

            if ((belowIndex[0] >= 0) ^ (belowIndex[1] >= 0) ^ (belowIndex[2] >= 0)) {
                maxClippedVerts += 2;
            }
        }
        clippedVerts.Reserve(maxClippedVerts);

        for (int baseIndex = 0; baseIndex < numSrcIndexes; baseIndex += 3) {
            int srcIndex[3];
            srcIndex[0] = srcIndexes[baseIndex + 0];
            srcIndex[1] = srcIndexes[baseIndex + 1];
            srcIndex[2] = srcIndexes[baseIndex + 2];
            int belowIndex[3] = { -1, -1, -1 };

            srcToBelowVertIndex.Get(srcIndex[0], &belowIndex[0]);
            srcToBelowVertIndex.Get(srcIndex[1], &belowIndex[1]);
            srcToBelowVertIndex.Get(srcIndex[2], &belowIndex[2]);

            if (belowIndex[0] >= 0 && belowIndex[1] >= 0 && belowIndex[2] >= 0) {
                // All vertices are below the plane.
                tempBelowIndexes.Append(belowIndex[0]);
                tempBelowIndexes.Append(belowIndex[1]);
                tempBelowIndexes.Append(belowIndex[2]);
            } else if (belowIndex[0] < 0 && belowIndex[1] < 0 && belowIndex[2] < 0) {
                // All vertices are above the plane.
                if (generateAboveMesh) {
                    int outsideIndex[3] = { -1, -1, -1 };
                    srcToAboveVertIndex.Get(srcIndex[0], &outsideIndex[0]);
                    srcToAboveVertIndex.Get(srcIndex[1], &outsideIndex[1]);
                    srcToAboveVertIndex.Get(srcIndex[2], &outsideIndex[2]);

                    tempAboveIndexes.Append(outsideIndex[0]);
                    tempAboveIndexes.Append(outsideIndex[1]);
                    tempAboveIndexes.Append(outsideIndex[2]);
                }
            } else {
                VertIndex belowTriFan[4];
                int numBelowTriFanIndex = 0;

                VertIndex aboveTriFan[4];
                int numAboveTriFanIndex = 0;

                // If partially being below the plane, clip to create 1 or 2 new triangles.
                for (int i = 0; i < 3; i++) {
                    int localCurrentVertex = i % 3;
                    int localNextVertex = (localCurrentVertex + 1) % 3;
                    bool shouldSlice;

                    if (belowIndex[localCurrentVertex] >= 0) {
                        belowTriFan[numBelowTriFanIndex++] = belowIndex[localCurrentVertex];

                        shouldSlice = belowIndex[localNextVertex] < 0;
                    } else {
                        if (generateAboveMesh) {
                            int outsideIndex = -1;
                            srcToAboveVertIndex.Get(srcIndex[localCurrentVertex], &outsideIndex);
                            aboveTriFan[numAboveTriFanIndex++] = outsideIndex;
                        }

                        shouldSlice = belowIndex[localNextVertex] >= 0;
                    }

                    if (shouldSlice) {
                        int currentVertexIndex = srcIndex[localCurrentVertex];
                        int nextVertexIndex = srcIndex[localNextVertex];

                        float currentVertexDist = vertexDistances[currentVertexIndex];
                        float nextVertexdist = vertexDistances[nextVertexIndex];
                        float t = currentVertexDist / (currentVertexDist - nextVertexdist);

                        const VertexGenericLit *v0 = &srcVerts[currentVertexIndex];
                        const VertexGenericLit *v1 = &srcVerts[nextVertexIndex];

                        VertexGenericLit clippedVertex;
                        clippedVertex.SetPosition(Math::Lerp(v0->GetPosition(), v1->GetPosition(), t));
                        clippedVertex.SetNormal(Math::Lerp(v0->GetNormal(), v1->GetNormal(), t));
                        clippedVertex.SetTexCoord(Math::Lerp(v0->GetTexCoord(), v1->GetTexCoord(), t));
                        clippedVertex.SetFloatColor(Math::Lerp(v0->GetFloatColor(), v1->GetFloatColor(), t));

                        VertIndex belowClippedVertexIndex = tempBelowVerts.Append(clippedVertex);
                        belowTriFan[numBelowTriFanIndex++] = belowClippedVertexIndex;

                        if (generateAboveMesh) {
                            aboveTriFan[numAboveTriFanIndex++] = tempAboveVerts.Append(clippedVertex);
                        }

                        // TODO: Optimization
                        bool foundSameClippedVertex = false;
                        for (int clippedVertIndex = 0; clippedVertIndex < clippedVerts.Count(); clippedVertIndex++) {
                            if (clippedVerts[clippedVertIndex].Equals(clippedVertex.GetPosition(), 0.00001f)) {
                                foundSameClippedVertex = true;
                                break;
                            }
                        }
                        if (!foundSameClippedVertex) {
                            clippedVerts.Append(clippedVertex.GetPosition());
                        }
                    }
                }

                tempBelowIndexes.Append(belowTriFan[0]);
                tempBelowIndexes.Append(belowTriFan[1]);
                tempBelowIndexes.Append(belowTriFan[2]);

                if (numBelowTriFanIndex > 3) {
                    tempBelowIndexes.Append(belowTriFan[0]);
                    tempBelowIndexes.Append(belowTriFan[2]);
                    tempBelowIndexes.Append(belowTriFan[3]);
                }

                if (generateAboveMesh) {
                    tempAboveIndexes.Append(aboveTriFan[0]);
                    tempAboveIndexes.Append(aboveTriFan[1]);
                    tempAboveIndexes.Append(aboveTriFan[2]);

                    if (numAboveTriFanIndex > 3) {
                        tempAboveIndexes.Append(aboveTriFan[0]);
                        tempAboveIndexes.Append(aboveTriFan[2]);
                        tempAboveIndexes.Append(aboveTriFan[3]);
                    }
                }
            }
        }

        if (generateCap && clippedVerts.Count() >= 3) {
            Array<VertexGenericLit> *tempBelowCapVertsPtr = &tempBelowVerts;
            Array<VertexGenericLit> *tempAboveCapVertsPtr = &tempAboveVerts;
            Array<VertIndex> *tempBelowCapIndexesPtr = &tempBelowIndexes;
            Array<VertIndex> *tempAboveCapIndexesPtr = &tempAboveIndexes;

            int belowCapBaseVertex = tempBelowCapVertsPtr->Count();
            int aboveCapBaseVertex = tempAboveCapVertsPtr->Count();

            Vec3 planeOrigin = slicePlane.Project(srcMesh.GetAABB().Center());
            //Vec3 planeOrigin = clippedVerts[0];
            Vec3 planeXAxis, planeYAxis;
            slicePlane.normal.OrthogonalBasis(planeXAxis, planeYAxis);
            planeXAxis = -planeXAxis;

            // pointsOnPlane has same order with clippedVerts.
            Array<Vec2> pointsOnPlane;
            pointsOnPlane.SetCount(clippedVerts.Count());

            for (int i = 0; i < clippedVerts.Count(); i++) {
                Vec3 offset = clippedVerts[i] - planeOrigin;
                pointsOnPlane[i].x = planeXAxis.Dot(offset);
                pointsOnPlane[i].y = planeYAxis.Dot(offset);
            }

            IDelaBella2<float> *idb = IDelaBella2<float>::Create();
            int numTriangulatedVerts = idb->Triangulate(pointsOnPlane.Count(), &pointsOnPlane[0].x, &pointsOnPlane[0].y, sizeof(Vec2));
            if (numTriangulatedVerts > 0) {
                for (int i = 0; i < clippedVerts.Count(); i++) {
                    const IDelaBella2<float>::Vertex *v = idb->GetVertexByIndex(i);

                    // Texture coordinates are based on the 2D coordinates used for triangulation.
                    // During triangulation, coordinates are normalized to [-1, 1], so need to multiply
                    // by normalization scale factor to get back to the appropriate scale.
                    Vec2 st = Vec2(v->x, v->y) * 0.5f + 0.5f;
                    st *= capTextureScale;

                    VertexGenericLit capVertex;
                    capVertex.SetPosition(clippedVerts[v->i]);
                    capVertex.SetNormal(slicePlane.normal);
                    capVertex.SetTexCoord(st);
                    capVertex.SetFloatColor(Color4::white);
                    tempBelowCapVertsPtr->Append(capVertex);

                    if (generateAboveMesh) {
                        capVertex.SetNormal(-capVertex.GetNormal());

                        tempAboveCapVertsPtr->Append(capVertex);
                    }
                }

                const IDelaBella2<float>::Simplex *simplex = idb->GetFirstDelaunaySimplex();
                int numTriangulatedTris = idb->GetNumPolygons();

                for (int i = 0; i < numTriangulatedTris; i++) {
                    tempBelowCapIndexesPtr->Append(belowCapBaseVertex + simplex->v[0]->i);
                    tempBelowCapIndexesPtr->Append(belowCapBaseVertex + simplex->v[1]->i);
                    tempBelowCapIndexesPtr->Append(belowCapBaseVertex + simplex->v[2]->i);

                    if (generateAboveMesh) {
                        tempAboveCapIndexesPtr->Append(aboveCapBaseVertex + simplex->v[2]->i);
                        tempAboveCapIndexesPtr->Append(aboveCapBaseVertex + simplex->v[1]->i);
                        tempAboveCapIndexesPtr->Append(aboveCapBaseVertex + simplex->v[0]->i);
                    }
                    simplex = simplex->next;
                }
            }
            idb->Destroy();
        }

        if (tempBelowVerts.Count() > 0 && tempBelowIndexes.Count() > 0) {
            MeshSurf *surf = outBelowMesh->AllocSurface(tempBelowVerts.Count(), tempBelowIndexes.Count());
            surf->materialIndex = srcSurf->materialIndex;
            outBelowMesh->surfaces.Append(surf);

            simdProcessor->Memcpy(surf->subMesh->verts, tempBelowVerts.Ptr(), sizeof(tempBelowVerts[0]) * tempBelowVerts.Count());
            simdProcessor->Memcpy(surf->subMesh->indexes, tempBelowIndexes.Ptr(), sizeof(tempBelowIndexes[0]) * tempBelowIndexes.Count());
        }

        if (tempAboveVerts.Count() > 0 && tempAboveIndexes.Count() > 0) {
            MeshSurf *surf = outAboveMesh->AllocSurface(tempAboveVerts.Count(), tempAboveIndexes.Count());
            surf->materialIndex = srcSurf->materialIndex;
            outAboveMesh->surfaces.Append(surf);

            simdProcessor->Memcpy(surf->subMesh->verts, tempAboveVerts.Ptr(), sizeof(tempAboveVerts[0]) * tempAboveVerts.Count());
            simdProcessor->Memcpy(surf->subMesh->indexes, tempAboveIndexes.Ptr(), sizeof(tempAboveIndexes[0]) * tempAboveIndexes.Count());
        }

        if (generateCap) {
            if (tempBelowCapVerts.Count() > 0 && tempBelowCapIndexes.Count() > 0) {
                MeshSurf *surf = outBelowMesh->AllocSurface(tempBelowCapVerts.Count(), tempBelowCapIndexes.Count());
                surf->materialIndex = srcSurf->materialIndex;
                outBelowMesh->surfaces.Append(surf);

                simdProcessor->Memcpy(surf->subMesh->verts, tempBelowCapVerts.Ptr(), sizeof(tempBelowCapVerts[0]) * tempBelowCapVerts.Count());
                simdProcessor->Memcpy(surf->subMesh->indexes, tempBelowCapIndexes.Ptr(), sizeof(tempBelowCapIndexes[0]) * tempBelowCapIndexes.Count());
            }

            if (tempAboveCapVerts.Count() > 0 && tempAboveCapIndexes.Count() > 0) {
                MeshSurf *surf = outAboveMesh->AllocSurface(tempAboveCapVerts.Count(), tempAboveCapIndexes.Count());
                surf->materialIndex = srcSurf->materialIndex;
                outAboveMesh->surfaces.Append(surf);

                simdProcessor->Memcpy(surf->subMesh->verts, tempAboveCapVerts.Ptr(), sizeof(tempAboveCapVerts[0]) * tempAboveCapVerts.Count());
                simdProcessor->Memcpy(surf->subMesh->indexes, tempAboveCapIndexes.Ptr(), sizeof(tempAboveCapIndexes[0]) *tempAboveCapIndexes.Count());
            }
        }
    }

    if (outBelowMesh->surfaces.Count() == 0 || outAboveMesh->surfaces.Count() == 0) {
        return false;
    }
    outBelowMesh->FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);

    if (generateAboveMesh) {
        outAboveMesh->FinishSurfaces(FinishFlag::ComputeAABB | FinishFlag::ComputeTangents);
    }

    return true;
}

BE_NAMESPACE_END
