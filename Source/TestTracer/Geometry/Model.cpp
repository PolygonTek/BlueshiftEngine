#include "Precompiled.h"
#include "Geometry/Model.h"
#include "Geometry/BVH.h"
#include "Geometry/Mesh.h"
#include "Shading/Material.h"
#include "Shading/Matte.h"
#include "Shading/ObjMat.h"
#include "Geometry/ObjLoader.h"

SubMesh *Model::AllocSubMesh(int numVerts, int numIndexes) {
    auto mesh = std::make_unique<SubMesh>();
    mesh->AllocSubMesh(numVerts, numIndexes);
    SubMesh *result = mesh.get();
    meshList.push_back(std::move(mesh));
    return result;
}

void Model::FinishMeshes() {
    for (const auto &mesh : meshList) {
        mesh->ComputeSmoothNormals();
    }
}

void Model::LoadFromObj(const char *filename) {
    ObjLoader objLoader(filename);

    for (size_t i = 0; i < objLoader.groupList.Count(); i++) {
        ObjGroup &group = objLoader.groupList[i];
        if (group.f_start == group.f_end) {
            continue;
        }
        BE1::Array<Vert> vertList;
        BE1::Array<int> indexList;
        BE1::HashMap<ObjFace, int, BE1::HashCompareDefault, HashGeneratorObjFace> vertMap;
        int maxIndex = -1;
        bool vtExist = false;
        bool vnExist = false;

        for (unsigned int j = group.f_start; j < group.f_end; j++) {
            const ObjFace *f = &objLoader.fList[j];

            Vert vert{};
            vert.xyz = objLoader.vList[f->v_index];

            if (f->t_index >= 0) {
                vert.st = objLoader.vtList[f->t_index];
                vtExist = true;
            }

            if (f->n_index >= 0) {
                vert.normal = objLoader.vnList[f->n_index];
                vnExist = true;
            }

            if (f->v_index > maxIndex) {
                maxIndex = f->v_index;
                vertList.Append(vert);
                vertMap[*f] = (int)vertList.Count() - 1;
                indexList.Append((int)vertList.Count() - 1);
            } else {
                unsigned int index;
                const auto *entry = vertMap.Get(*f);
                if (entry) {
                    index = entry->second;
                } else {
                    vertList.Append(vert);
                    vertMap[*f] = (int)vertList.Count() - 1;
                    index = (int)vertList.Count() - 1;
                }
                indexList.Append(index);
            }
        }

        SubMesh *mesh = AllocSubMesh((int)vertList.Count(), (int)indexList.Count());
        for (unsigned int j = 0; j < vertList.Count(); j++) {
            mesh->verts[j].xyz = vertList[j].xyz;
            if (vtExist) {
                mesh->verts[j].st = vertList[j].st;
            }
            if (vnExist) {
                mesh->verts[j].normal = vertList[j].normal;
            }
        }

        for (unsigned int j = 0; j < indexList.Count(); j++) {
            mesh->indexes[j] = indexList[j];
        }

        if (!vnExist) {
            mesh->ComputeSmoothNormals();
        }

        ObjMtl *mtl = nullptr;
        objLoader.mtlMap.Get(group.mtl_name.c_str(), &mtl);
        if (mtl) {
            if (group.mtl_name == "light") {
                mesh->material = new EmissiveMaterial(mtl->Kd);
            } else {
                //std::string s = filename;
                //std::string basedir = s.substr(0, s.find_last_of("\\/") + 1);
                BE1::Str basedir = filename;
                basedir.StripFileName();
                mesh->material = new ObjMaterial(*mtl, basedir.c_str());
            }
        } else {
            mesh->material = new MatteMaterial(BE1::Vec3(1.0f), 0.0f); //
        }
        importedMaterials.emplace_back(mesh->material);
    }
}

void Model::BuildBVH() {
    bvh = std::make_unique<BVH>();
    bvh->Build(this);
}
