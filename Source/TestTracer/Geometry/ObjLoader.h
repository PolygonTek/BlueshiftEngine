#pragma once

#include "BlueshiftEngine.h"

struct ObjFace {
    int                             v_index;
    int                             t_index;
    int                             n_index;

    bool                            operator==(const ObjFace &rhs) const {
        return (v_index == rhs.v_index) && (t_index == rhs.t_index) && (n_index == rhs.n_index);
    }
};

struct HashGeneratorObjFace {
    static int                      Hash(const BE1::HashIndex &hasher, const ObjFace &value) {
        return (value.v_index + value.t_index + value.n_index) % (hasher.GetHashSize() - 1);
    }
};

struct ObjMtl {
    float                           d;
    float                           Ns;
    BE1::Vec3                       Ka;
    BE1::Vec3                       Kd;
    BE1::Vec3                       Ks;
    BE1::Vec3                       Tf;
    BE1::Str                        map_d;
    BE1::Str                        map_Ns;
    BE1::Str                        map_Ka;
    BE1::Str                        map_Kd;
    BE1::Str                        map_Ks;
    BE1::Str                        map_bump;

    void                            Clear() {
        d = Ns = 0.0f;
        Ka.SetFromScalar(0);
        Kd.SetFromScalar(0);
        Ks.SetFromScalar(0);
        Tf.SetFromScalar(0);
        map_d.Clear();
        map_Ns.Clear();
        map_Ka.Clear();
        map_Kd.Clear();
        map_Ks.Clear();
        map_bump.Clear();
    }
};

struct ObjGroup {
    unsigned int                    f_start;
    unsigned int                    f_end;
    BE1::Str                        mtl_name;
};

class ObjLoader {
public:
    ObjLoader(const char *filename);

    BE1::Array<BE1::Vec3>           vList;
    BE1::Array<BE1::Vec3>           vnList;
    BE1::Array<BE1::Vec2>           vtList;
    BE1::Array<ObjFace>             fList;
    BE1::HashTable<BE1::Str, ObjMtl> mtlMap;
    BE1::Array<ObjGroup>            groupList;

private:
    void                            LoadObj(const char *filename);
    void                            LoadMtl(const char *filename);
};

BE_INLINE ObjLoader::ObjLoader(const char *filename) {
    LoadObj(filename);
}
